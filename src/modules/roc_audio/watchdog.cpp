/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/watchdog.h"
#include "roc_core/log.h"
#include "roc_error/error_code.h"

namespace roc {
namespace audio {

Watchdog::Watchdog(IReader& reader,
                   const size_t num_channels,
                   const WatchdogConfig& config,
                   size_t sample_rate,
                   core::IAllocator& allocator)
    : reader_(reader)
    , num_channels_(num_channels)
    , max_blank_duration_((packet::timestamp_t)packet::timestamp_from_ns(
          config.no_playback_timeout, sample_rate))
    , max_drops_duration_((packet::timestamp_t)packet::timestamp_from_ns(
          config.broken_playback_timeout, sample_rate))
    , drop_detection_window_((packet::timestamp_t)packet::timestamp_from_ns(
          config.breakage_detection_window, sample_rate))
    , curr_read_pos_(0)
    , last_pos_before_blank_(0)
    , last_pos_before_drops_(0)
    , curr_window_flags_(0)
    , status_(allocator)
    , status_pos_(0)
    , status_show_(false)
    , alive_(true)
    , valid_(false) {
    if (config.no_playback_timeout < 0 || config.broken_playback_timeout < 0
        || config.breakage_detection_window < 0) {
        roc_log(LogError,
                "watchdog: invalid config: "
                "no_packets_timeout=%ld drops_timeout=%ld drop_detection_window=%ld",
                (long)config.no_playback_timeout, (long)config.broken_playback_timeout,
                (long)config.breakage_detection_window);
        return;
    }

    if (max_drops_duration_ != 0) {
        if (drop_detection_window_ == 0 || drop_detection_window_ > max_drops_duration_) {
            roc_log(LogError,
                    "watchdog: invalid config: "
                    "drop_detection_window should be in range (0; max_drops_duration]: "
                    "max_drops_duration=%lu drop_detection_window=%lu",
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
            "watchdog: initializing: "
            "max_blank_duration=%lu max_drops_duration=%lu drop_detection_window=%lu",
            (unsigned long)max_blank_duration_, (unsigned long)max_drops_duration_,
            (unsigned long)drop_detection_window_);

    valid_ = true;
}

bool Watchdog::valid() const {
    return valid_;
}

ssize_t Watchdog::read(Frame& frame) {
    if (!alive_) {
        if (frame.size() != 0) {
            memset(frame.data(), 0, frame.size() * sizeof(sample_t));
        }
        return 0;
    }

    ssize_t ret_val = reader_.read(frame);
    if (ret_val <= 0) {
        return ret_val;
    }

    const packet::timestamp_t next_read_pos =
        packet::timestamp_t(curr_read_pos_ + frame.size() / num_channels_);

    update_blank_timeout_(frame, next_read_pos);
    update_drops_timeout_(frame, next_read_pos);
    update_status_(frame);

    curr_read_pos_ = next_read_pos;

    if (!check_drops_timeout_()) {
        flush_status_();
        alive_ = false;
    }

    return ret_val;
}

bool Watchdog::update() {
    if (!alive_) {
        return false;
    }

    if (!check_blank_timeout_()) {
        flush_status_();
        alive_ = false;
        return false;
    }

    return true;
}

void Watchdog::update_blank_timeout_(const Frame& frame,
                                     packet::timestamp_t next_read_pos) {
    if (max_blank_duration_ == 0) {
        return;
    }

    if (frame.flags() & Frame::FlagBlank) {
        return;
    }

    last_pos_before_blank_ = next_read_pos;
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
                                     packet::timestamp_t next_read_pos) {
    if (max_drops_duration_ == 0) {
        return;
    }

    curr_window_flags_ |= frame.flags();

    const packet::timestamp_t window_start =
        curr_read_pos_ / drop_detection_window_ * drop_detection_window_;

    const packet::timestamp_t window_end = window_start + drop_detection_window_;

    if (packet::timestamp_le(window_end, next_read_pos)) {
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
    if (status_.size() == 0) {
        return;
    }

    const unsigned flags = frame.flags();

    char symbol = '.';

    if (flags & Frame::FlagBlank) {
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
