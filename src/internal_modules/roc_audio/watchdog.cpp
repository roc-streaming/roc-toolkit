/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/watchdog.h"
#include "roc_core/log.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

bool WatchdogConfig::deduce_defaults(const core::nanoseconds_t default_latency,
                                     const core::nanoseconds_t target_latency) {
    const core::nanoseconds_t configured_latency =
        target_latency != 0 ? target_latency : default_latency;

    if (no_playback_timeout == 0) {
        no_playback_timeout = configured_latency * 4 / 3;
    }

    if (choppy_playback_timeout == 0) {
        choppy_playback_timeout = 2 * core::Second;
    }

    if (choppy_playback_window == 0) {
        choppy_playback_window =
            std::min(300 * core::Millisecond, choppy_playback_timeout / 4);
    }

    if (warmup_duration == 0) {
        warmup_duration = configured_latency;
    }

    return true;
}

Watchdog::Watchdog(IFrameReader& reader,
                   const SampleSpec& sample_spec,
                   const WatchdogConfig& config,
                   core::IArena& arena)
    : reader_(reader)
    , sample_spec_(sample_spec)
    , max_blank_duration_(0)
    , max_drops_duration_(0)
    , drops_detection_window_(0)
    , curr_read_pos_(0)
    , last_pos_before_blank_(0)
    , last_pos_before_drops_(0)
    , warmup_duration_(0)
    , in_warmup_(false)
    , curr_window_flags_(0)
    , status_(arena)
    , status_pos_(0)
    , show_status_(false)
    , init_status_(status::NoStatus) {
    if (config.no_playback_timeout >= 0) {
        max_blank_duration_ =
            std::max(sample_spec_.ns_2_stream_timestamp(config.no_playback_timeout), 1u);
    }

    if (config.choppy_playback_timeout >= 0) {
        max_drops_duration_ = std::max(
            sample_spec_.ns_2_stream_timestamp(config.choppy_playback_timeout), 1u);

        drops_detection_window_ = std::max(
            sample_spec_.ns_2_stream_timestamp(config.choppy_playback_window), 1u);
    }

    if (config.warmup_duration >= 0) {
        warmup_duration_ =
            std::max(sample_spec_.ns_2_stream_timestamp(config.warmup_duration), 1u);
    }

    last_pos_before_blank_ = warmup_duration_;
    in_warmup_ = warmup_duration_ != 0;

    roc_log(LogDebug,
            "watchdog: initializing:"
            " max_blank_duration=%lu(%.3fms) max_drops_duration=%lu(%.3fms)"
            " drop_detection_window=%lu(%.3fms) warmup_duration=%lu(%.3fms)",
            (unsigned long)max_blank_duration_,
            sample_spec_.stream_timestamp_2_ms(max_blank_duration_),
            (unsigned long)max_drops_duration_,
            sample_spec_.stream_timestamp_2_ms(max_drops_duration_),
            (unsigned long)drops_detection_window_,
            sample_spec_.stream_timestamp_2_ms(drops_detection_window_),
            (unsigned long)warmup_duration_,
            sample_spec_.stream_timestamp_2_ms(warmup_duration_));

    if (max_drops_duration_ != 0
        && (drops_detection_window_ < 1
            || drops_detection_window_ > max_drops_duration_)) {
        roc_log(LogError,
                "watchdog: invalid config: drop_detection_window out of bounds");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (config.frame_status_window != 0) {
        if (!status_.resize(config.frame_status_window + 1)) {
            return;
        }
    }

    init_status_ = status::StatusOK;
}

status::StatusCode Watchdog::init_status() const {
    return init_status_;
}

status::StatusCode
Watchdog::read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    const status::StatusCode code = reader_.read(frame, duration, mode);
    if (code != status::StatusOK && code != status::StatusPart) {
        return code;
    }

    sample_spec_.validate_frame(frame);

    if (!update_(frame)) {
        return status::StatusAbort;
    }

    return code;
}

bool Watchdog::update_(Frame& frame) {
    const packet::stream_timestamp_t next_read_pos = curr_read_pos_ + frame.duration();

    update_blank_timeout_(frame, next_read_pos);
    update_drops_timeout_(frame, next_read_pos);
    update_status_(frame);

    curr_read_pos_ = next_read_pos;

    if (!check_drops_timeout_()) {
        flush_status_();
        return false;
    }

    if (!check_blank_timeout_()) {
        flush_status_();
        return false;
    }

    update_warmup_();

    return true;
}

void Watchdog::update_blank_timeout_(const Frame& frame,
                                     packet::stream_timestamp_t next_read_pos) {
    if (max_blank_duration_ == 0) {
        return;
    }

    if (frame.flags() & Frame::HasSignal) {
        last_pos_before_blank_ = next_read_pos;
        in_warmup_ = false;
    }
}

bool Watchdog::check_blank_timeout_() const {
    if (max_blank_duration_ == 0 || in_warmup_) {
        return true;
    }

    if (curr_read_pos_ - last_pos_before_blank_ < max_blank_duration_) {
        return true;
    }

    roc_log(LogDebug,
            "watchdog: no_playback timeout reached:"
            " every frame was blank during timeout:"
            " max_blank_duration=%lu(%.3fms) warmup_duration=%lu(%.3fms)",
            (unsigned long)max_blank_duration_,
            sample_spec_.stream_timestamp_2_ms(max_blank_duration_),
            (unsigned long)warmup_duration_,
            sample_spec_.stream_timestamp_2_ms(warmup_duration_));

    return false;
}

void Watchdog::update_drops_timeout_(const Frame& frame,
                                     packet::stream_timestamp_t next_read_pos) {
    if (max_drops_duration_ == 0) {
        return;
    }

    curr_window_flags_ |= frame.flags();

    const packet::stream_timestamp_t window_start =
        curr_read_pos_ / drops_detection_window_ * drops_detection_window_;

    const packet::stream_timestamp_t window_end = window_start + drops_detection_window_;

    if (packet::stream_timestamp_le(window_end, next_read_pos)) {
        const unsigned drop_flags = Frame::HasGaps | Frame::HasDrops;

        if ((curr_window_flags_ & drop_flags) != drop_flags) {
            last_pos_before_drops_ = next_read_pos;
        }

        if (next_read_pos % drops_detection_window_ == 0) {
            curr_window_flags_ = 0;
        } else {
            curr_window_flags_ = frame.flags();
        }
    }
}

bool Watchdog::check_drops_timeout_() {
    if (max_drops_duration_ == 0) {
        return true;
    }

    if (curr_read_pos_ - last_pos_before_drops_ < max_drops_duration_) {
        return true;
    }

    roc_log(LogDebug,
            "watchdog: choppy_playback timeout reached:"
            " every window had frames with packet drops during timeout:"
            " max_drops_duration=%lu(%.3fms) drop_detection_window=%lu(%.3fms)",
            (unsigned long)max_drops_duration_,
            sample_spec_.stream_timestamp_2_ms(max_drops_duration_),
            (unsigned long)drops_detection_window_,
            sample_spec_.stream_timestamp_2_ms(drops_detection_window_));

    return false;
}

void Watchdog::update_warmup_() {
    in_warmup_ = in_warmup_ && (curr_read_pos_ < warmup_duration_);
}

void Watchdog::update_status_(const Frame& frame) {
    if (status_.is_empty()) {
        return;
    }

    const unsigned flags = frame.flags();

    char symbol = '.';

    if (!(flags & Frame::HasSignal)) {
        if (in_warmup_) {
            if (flags & Frame::HasDrops) {
                symbol = 'W';
            } else {
                symbol = 'w';
            }
        } else {
            if (flags & Frame::HasDrops) {
                symbol = 'B';
            } else {
                symbol = 'b';
            }
        }
    } else if (flags & Frame::HasGaps) {
        if (flags & Frame::HasDrops) {
            symbol = 'I';
        } else {
            symbol = 'i';
        }
    } else if (flags & Frame::HasDrops) {
        symbol = 'D';
    }

    status_[status_pos_] = symbol;
    status_pos_++;
    show_status_ = show_status_ || symbol != '.';

    if (status_pos_ == status_.size() - 1) {
        flush_status_();
    }
}

void Watchdog::flush_status_() {
    if (status_pos_ == 0) {
        return;
    }

    if (show_status_) {
        for (; status_pos_ < status_.size(); status_pos_++) {
            status_[status_pos_] = '\0';
        }
        roc_log(LogDebug, "watchdog: status: %s", &status_[0]);
    }

    status_pos_ = 0;
    show_status_ = false;
}

} // namespace audio
} // namespace roc
