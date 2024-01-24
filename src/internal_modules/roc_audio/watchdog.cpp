/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/watchdog.h"
#include "roc_core/log.h"

namespace roc {
namespace audio {

void WatchdogConfig::deduce_defaults(const core::nanoseconds_t target_latency) {
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
}

Watchdog::Watchdog(IFrameReader& reader,
                   const audio::SampleSpec& sample_spec,
                   const WatchdogConfig& config,
                   core::IArena& arena)
    : reader_(reader)
    , sample_spec_(sample_spec)
    , max_blank_duration_(
          (packet::stream_timestamp_t)sample_spec.ns_2_stream_timestamp_delta(
              config.no_playback_timeout))
    , max_drops_duration_(
          (packet::stream_timestamp_t)sample_spec.ns_2_stream_timestamp_delta(
              config.choppy_playback_timeout))
    , drop_detection_window_(
          (packet::stream_timestamp_t)sample_spec.ns_2_stream_timestamp_delta(
              config.choppy_playback_window))
    , curr_read_pos_(0)
    , last_pos_before_blank_(0)
    , last_pos_before_drops_(0)
    , curr_window_flags_(0)
    , status_(arena)
    , status_pos_(0)
    , status_show_(false)
    , alive_(true)
    , valid_(false) {
    if (config.no_playback_timeout < 0 || config.choppy_playback_timeout < 0
        || config.choppy_playback_window < 0) {
        roc_log(LogError,
                "watchdog: invalid config:"
                " no_packets_timeout=%ld drops_timeout=%ld drop_detection_window=%ld",
                (long)config.no_playback_timeout, (long)config.choppy_playback_timeout,
                (long)config.choppy_playback_window);
        return;
    }

    if (max_drops_duration_ != 0) {
        if (drop_detection_window_ == 0 || drop_detection_window_ > max_drops_duration_) {
            roc_log(LogError,
                    "watchdog: invalid config:"
                    " drop_detection_window should be in range (0; max_drops_duration]:"
                    " max_drops_duration=%lu drop_detection_window=%lu",
                    (unsigned long)max_drops_duration_,
                    (unsigned long)drop_detection_window_);
            return;
        }
    }

    if (config.frame_status_window != 0) {
        if (!status_.resize(config.frame_status_window + 1)) {
            return;
        }
    }

    roc_log(LogDebug,
            "watchdog: initializing:"
            " max_blank_duration=%lu max_drops_duration=%lu drop_detection_window=%lu",
            (unsigned long)max_blank_duration_, (unsigned long)max_drops_duration_,
            (unsigned long)drop_detection_window_);

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
        if (frame.num_samples() != 0) {
            memset(frame.samples(), 0, frame.num_samples() * sizeof(sample_t));
        }
        return true;
    }

    if (!reader_.read(frame)) {
        return false;
    }

    const packet::stream_timestamp_t next_read_pos = packet::stream_timestamp_t(
        curr_read_pos_ + frame.num_samples() / sample_spec_.num_channels());

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

    return true;
}

void Watchdog::update_blank_timeout_(const Frame& frame,
                                     packet::stream_timestamp_t next_read_pos) {
    if (max_blank_duration_ == 0) {
        return;
    }

    if (frame.flags() & Frame::FlagNonblank) {
        last_pos_before_blank_ = next_read_pos;
    }
}

bool Watchdog::check_blank_timeout_() const {
    if (max_blank_duration_ == 0) {
        return true;
    }

    if (curr_read_pos_ - last_pos_before_blank_ < max_blank_duration_) {
        return true;
    }

    roc_log(LogDebug,
            "watchdog: blank timeout reached: every frame was blank during timeout:"
            " curr_read_pos=%lu last_pos_before_blank=%lu max_blank_duration=%lu",
            (unsigned long)curr_read_pos_, (unsigned long)last_pos_before_blank_,
            (unsigned long)max_blank_duration_);

    return false;
}

void Watchdog::update_drops_timeout_(const Frame& frame,
                                     packet::stream_timestamp_t next_read_pos) {
    if (max_drops_duration_ == 0) {
        return;
    }

    curr_window_flags_ |= frame.flags();

    const packet::stream_timestamp_t window_start =
        curr_read_pos_ / drop_detection_window_ * drop_detection_window_;

    const packet::stream_timestamp_t window_end = window_start + drop_detection_window_;

    if (packet::stream_timestamp_le(window_end, next_read_pos)) {
        const unsigned drop_flags = Frame::FlagIncomplete | Frame::FlagDrops;

        if ((curr_window_flags_ & drop_flags) != drop_flags) {
            last_pos_before_drops_ = next_read_pos;
        }

        if (next_read_pos % drop_detection_window_ == 0) {
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
            "watchdog: drops timeout reached: every window had drops during timeout:"
            " curr_read_pos=%lu last_pos_before_drops=%lu"
            " drop_detection_window=%lu max_drops_duration=%lu",
            (unsigned long)curr_read_pos_, (unsigned long)last_pos_before_drops_,
            (unsigned long)drop_detection_window_, (unsigned long)max_drops_duration_);

    return false;
}

void Watchdog::update_status_(const Frame& frame) {
    if (status_.is_empty()) {
        return;
    }

    const unsigned flags = frame.flags();

    char symbol = '.';

    if (!(flags & Frame::FlagNonblank)) {
        if (flags & Frame::FlagDrops) {
            symbol = 'B';
        } else {
            symbol = 'b';
        }
    } else if (flags & Frame::FlagIncomplete) {
        if (flags & Frame::FlagDrops) {
            symbol = 'I';
        } else {
            symbol = 'i';
        }
    } else if (flags & Frame::FlagDrops) {
        symbol = 'D';
    }

    status_[status_pos_] = symbol;
    status_pos_++;
    status_show_ = status_show_ || symbol != '.';

    if (status_pos_ == status_.size() - 1) {
        flush_status_();
    }
}

void Watchdog::flush_status_() {
    if (status_pos_ == 0) {
        return;
    }

    if (status_show_) {
        for (; status_pos_ < status_.size(); status_pos_++) {
            status_[status_pos_] = '\0';
        }
        roc_log(LogDebug, "watchdog: status: %s", &status_[0]);
    }

    status_pos_ = 0;
    status_show_ = false;
}

} // namespace audio
} // namespace roc
