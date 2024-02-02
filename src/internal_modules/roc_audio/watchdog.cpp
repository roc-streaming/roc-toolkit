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

namespace {

double timestamp_to_ms(const SampleSpec& sample_spec,
                       packet::stream_timestamp_t timestamp) {
    return (double)sample_spec.stream_timestamp_2_ns(timestamp) / core::Millisecond;
}

} // namespace

void WatchdogConfig::deduce_defaults(core::nanoseconds_t target_latency) {
    if (target_latency <= 0) {
        target_latency = 200 * core::Millisecond;
    }

    if (no_playback_timeout < 0) {
        no_playback_timeout = target_latency * 4 / 3;
    }

    if (choppy_playback_timeout < 0) {
        choppy_playback_timeout = 2 * core::Second;
    }

    if (choppy_playback_window < 0) {
        choppy_playback_window =
            std::min(300 * core::Millisecond, choppy_playback_timeout / 4);
    }

    if (warmup_duration < 0) {
        warmup_duration = target_latency;
    }
}

Watchdog::Watchdog(IFrameReader& reader,
                   const audio::SampleSpec& sample_spec,
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
    , alive_(true)
    , valid_(false) {
    if (config.no_playback_timeout < 0 || config.choppy_playback_timeout < 0
        || config.choppy_playback_window < 0 || config.warmup_duration < 0) {
        roc_log(LogError,
                "watchdog: invalid config: negative duration:"
                " no_playback_timeout=%.3fms choppy_playback_timeout=%.3fms"
                " choppy_playback_window=%.3fms warmup_duration=%.3fms",
                (double)config.no_playback_timeout / core::Millisecond,
                (double)config.choppy_playback_timeout / core::Millisecond,
                (double)config.choppy_playback_window / core::Millisecond,
                (double)config.warmup_duration / core::Millisecond);
        return;
    }

    max_blank_duration_ = sample_spec_.ns_2_stream_timestamp(config.no_playback_timeout);
    max_drops_duration_ =
        sample_spec_.ns_2_stream_timestamp(config.choppy_playback_timeout);
    drops_detection_window_ =
        sample_spec_.ns_2_stream_timestamp(config.choppy_playback_window);
    warmup_duration_ = sample_spec_.ns_2_stream_timestamp(config.warmup_duration);

    last_pos_before_blank_ = warmup_duration_;
    in_warmup_ = warmup_duration_ != 0;

    if (max_drops_duration_ != 0
        && (drops_detection_window_ < 1
            || drops_detection_window_ > max_drops_duration_)) {
        roc_log(LogError,
                "watchdog: invalid config: choppy_playback_window out of bounds:"
                " no_playback_timeout=%.3fms choppy_playback_timeout=%.3fms"
                " choppy_playback_window=%.3fms warmup_duration=%.3fms",
                (double)config.no_playback_timeout / core::Millisecond,
                (double)config.choppy_playback_timeout / core::Millisecond,
                (double)config.choppy_playback_window / core::Millisecond,
                (double)config.warmup_duration / core::Millisecond);
        return;
    }

    if (config.frame_status_window != 0) {
        if (!status_.resize(config.frame_status_window + 1)) {
            return;
        }
    }

    roc_log(LogDebug,
            "watchdog: initializing:"
            " max_blank_duration=%lu(%.3fms) max_drops_duration=%lu(%.3fms)"
            " drop_detection_window=%lu(%.3fms) warmup_duration=%lu(%.3fms)",
            (unsigned long)max_blank_duration_,
            timestamp_to_ms(sample_spec_, max_blank_duration_),
            (unsigned long)max_drops_duration_,
            timestamp_to_ms(sample_spec_, max_drops_duration_),
            (unsigned long)drops_detection_window_,
            timestamp_to_ms(sample_spec_, drops_detection_window_),
            (unsigned long)warmup_duration_,
            timestamp_to_ms(sample_spec_, warmup_duration_));

    valid_ = true;
}

bool Watchdog::is_valid() const {
    return valid_;
}

bool Watchdog::is_alive() const {
    roc_panic_if(!is_valid());

    return alive_;
}

bool Watchdog::read(Frame& frame) {
    roc_panic_if(!is_valid());

    if (!alive_) {
        if (frame.num_bytes() != 0) {
            memset(frame.bytes(), 0, frame.num_bytes());
        }
        return true;
    }

    if (!reader_.read(frame)) {
        return false;
    }

    const packet::stream_timestamp_t next_read_pos = curr_read_pos_ + frame.duration();

    update_blank_timeout_(frame, next_read_pos);
    update_drops_timeout_(frame, next_read_pos);
    update_status_(frame);

    curr_read_pos_ = next_read_pos;

    if (!check_drops_timeout_()) {
        flush_status_();
        alive_ = false;
    }

    if (!check_blank_timeout_()) {
        flush_status_();
        alive_ = false;
    }

    update_warmup_();

    return true;
}

void Watchdog::update_blank_timeout_(const Frame& frame,
                                     packet::stream_timestamp_t next_read_pos) {
    if (max_blank_duration_ == 0) {
        return;
    }

    if (frame.flags() & Frame::FlagNotBlank) {
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
            timestamp_to_ms(sample_spec_, max_blank_duration_),
            (unsigned long)warmup_duration_,
            timestamp_to_ms(sample_spec_, warmup_duration_));

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
        const unsigned drop_flags = Frame::FlagNotComplete | Frame::FlagPacketDrops;

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
            timestamp_to_ms(sample_spec_, max_drops_duration_),
            (unsigned long)drops_detection_window_,
            timestamp_to_ms(sample_spec_, drops_detection_window_));

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

    if (!(flags & Frame::FlagNotBlank)) {
        if (in_warmup_) {
            if (flags & Frame::FlagPacketDrops) {
                symbol = 'B';
            } else {
                symbol = 'b';
            }
        } else {
            if (flags & Frame::FlagPacketDrops) {
                symbol = 'W';
            } else {
                symbol = 'w';
            }
        }
    } else if (flags & Frame::FlagNotComplete) {
        if (flags & Frame::FlagPacketDrops) {
            symbol = 'I';
        } else {
            symbol = 'i';
        }
    } else if (flags & Frame::FlagPacketDrops) {
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
