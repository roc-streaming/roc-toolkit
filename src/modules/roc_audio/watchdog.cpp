/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/watchdog.h"
#include "roc_core/log.h"

namespace roc {
namespace audio {

Watchdog::Watchdog(IReader& reader,
                   const size_t num_channels,
                   const WatchdogConfig& config,
                   core::IAllocator& allocator)
    : reader_(reader)
    , num_channels_(num_channels)
    , max_silence_duration_(config.silence_timeout)
    , max_drops_duration_(config.drops_timeout)
    , drop_detection_window_(config.drop_detection_window)
    , curr_read_pos_(0)
    , last_pos_before_silence_(0)
    , last_pos_before_drops_(0)
    , curr_window_flags_(0)
    , first_update_pos_(0)
    , have_first_update_pos_(false)
    , status_(allocator)
    , status_pos_(0)
    , status_show_(false)
    , alive_(true)
    , valid_(false) {
    if (max_drops_duration_ != 0) {
        if (drop_detection_window_ == 0 || drop_detection_window_ > max_drops_duration_) {
            roc_log(LogDebug,
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
            "max_silence_duration=%lu max_drops_duration=%lu drop_detection_window=%lu",
            (unsigned long)max_silence_duration_, (unsigned long)max_drops_duration_,
            (unsigned long)drop_detection_window_);

    valid_ = true;
}

bool Watchdog::valid() const {
    return valid_;
}

void Watchdog::read(Frame& frame) {
    if (!alive_) {
        if (frame.size() != 0) {
            memset(frame.data(), 0, frame.size() * sizeof(sample_t));
        }
        return;
    }

    reader_.read(frame);

    const packet::timestamp_t next_read_pos =
        packet::timestamp_t(curr_read_pos_ + frame.size() / num_channels_);

    update_silence_timeout_(frame, next_read_pos);
    update_drops_timeout_(frame, next_read_pos);
    update_status_(frame);

    curr_read_pos_ = next_read_pos;

    if (!check_drops_timeout_()) {
        flush_status_();
        alive_ = false;
    }
}

bool Watchdog::update(packet::timestamp_t new_update_pos) {
    if (!alive_) {
        return false;
    }

    if (!have_first_update_pos_) {
        first_update_pos_ = new_update_pos;
        have_first_update_pos_ = true;
    }

    const packet::timestamp_t pos = new_update_pos - first_update_pos_;

    if (!check_silence_timeout_(pos)) {
        flush_status_();
        alive_ = false;
        return false;
    }

    return true;
}

void Watchdog::update_silence_timeout_(const Frame& frame, packet::timestamp_t next_read_pos) {
    if (max_silence_duration_ == 0) {
        return;
    }

    if (frame.flags() & audio::Frame::FlagBlank) {
        return;
    }

    last_pos_before_silence_ = next_read_pos;
}

bool Watchdog::check_silence_timeout_(packet::timestamp_t curr_update_pos) const {
    if (max_silence_duration_ == 0) {
        return true;
    }

    if (ROC_UNSIGNED_LE(packet::signed_timestamp_t, curr_update_pos,
                        last_pos_before_silence_)) {
        return true;
    }

    if (curr_update_pos - last_pos_before_silence_ < max_silence_duration_) {
        return true;
    }

    roc_log(LogDebug,
            "watchdog: silence timeout reached: every frame was blank during timeout:"
            " curr_update_pos=%lu last_pos_before_silence=%lu max_silence_duration=%lu",
            (unsigned long)curr_update_pos, (unsigned long)last_pos_before_silence_,
            (unsigned long)max_silence_duration_);

    return false;
}

void Watchdog::update_drops_timeout_(const Frame& frame, packet::timestamp_t next_read_pos) {
    if (max_drops_duration_ == 0) {
        return;
    }

    curr_window_flags_ |= frame.flags();

    const packet::timestamp_t window_start =
        curr_read_pos_ / drop_detection_window_ * drop_detection_window_;

    const packet::timestamp_t window_end =
        window_start + drop_detection_window_;

    if (ROC_UNSIGNED_LE(packet::signed_timestamp_t, window_end, next_read_pos)) {
        if ((curr_window_flags_ & (Frame::FlagIncomplete | Frame::FlagDrops)) == 0) {
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
            (unsigned long)drop_detection_window_,
            (unsigned long)max_drops_duration_);

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
