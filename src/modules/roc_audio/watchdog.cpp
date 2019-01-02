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
                   packet::timestamp_t max_silence_duration,
                   packet::timestamp_t max_drops_duration,
                   packet::timestamp_t drop_detection_window)
    : reader_(reader)
    , num_channels_(num_channels)
    , max_silence_duration_(max_silence_duration)
    , max_drops_duration_(max_drops_duration)
    , drop_detection_window_(drop_detection_window)
    , first_update_(true)
    , alive_(true)
    , curr_read_pos_(0)
    , last_update_time_(0)
    , last_update_before_silence_(0)
    , last_read_before_drops_(0)
    , drop_in_curr_window_(false) {
}

void Watchdog::read(Frame& frame) {
    if (!alive_) {
        return;
    }

    reader_.read(frame);

    const packet::timestamp_t next_read_pos =
        packet::timestamp_t(curr_read_pos_ + frame.size() / num_channels_);

    update_silence_timeout_(frame);
    update_drops_timeout_(frame, next_read_pos);

    curr_read_pos_ = next_read_pos;

    if (!check_drops_timeout_()) {
        alive_ = false;
    }
}

bool Watchdog::update(packet::timestamp_t time) {
    if (!alive_) {
        return false;
    }

    if (first_update_) {
        init_silence_timeout_(time);
        first_update_ = false;
    }

    last_update_time_ = time;

    if (!check_silence_timeout_()) {
        alive_ = false;
        return false;
    }

    return true;
}

void Watchdog::init_silence_timeout_(packet::timestamp_t update_time) {
    last_update_before_silence_ = update_time;
}

void Watchdog::update_silence_timeout_(const Frame& frame) {
    if (frame.flags() & audio::Frame::FlagEmpty) {
        return;
    }

    last_update_before_silence_ = last_update_time_;
}

bool Watchdog::check_silence_timeout_() const {
    if (last_update_time_ - last_update_before_silence_ < max_silence_duration_) {
        return true;
    }

    roc_log(LogInfo,
            "watchdog: timeout reached: every frame was empty during timeout:"
            " last_update_time=%lu last_update_before_empty=%lu max_silence_duration=%lu",
            (unsigned long)last_update_time_, (unsigned long)last_update_before_silence_,
            (unsigned long)max_silence_duration_);

    return false;
}

void Watchdog::update_drops_timeout_(const Frame& frame, packet::timestamp_t next_read_pos) {
    const unsigned flags = frame.flags();

    if ((flags & audio::Frame::FlagPacketDrops) && !(flags & audio::Frame::FlagFull)) {
        drop_in_curr_window_ = true;
    }

    const packet::timestamp_t window_start =
        curr_read_pos_ / drop_detection_window_ * drop_detection_window_;

    const packet::timestamp_t window_end =
        window_start + drop_detection_window_;

    const bool out_of_window =
        ROC_UNSIGNED_LE(packet::signed_timestamp_t, window_end, next_read_pos);

    if (out_of_window) {
        if (!drop_in_curr_window_) {
            last_read_before_drops_ = next_read_pos;
        }
        if (window_end == next_read_pos) {
            // reset flag if the frame does not affect new window
            drop_in_curr_window_ = false;
        }
    }
}

bool Watchdog::check_drops_timeout_() {
    if (curr_read_pos_ - last_read_before_drops_ < max_drops_duration_) {
        return true;
    }

    roc_log(LogInfo,
            "watchdog: timeout reached: every window had drops during timeout:"
            " curr_read_pos=%lu last_read_before_drops=%lu"
            " drop_detection_window=%lu max_drops_duration=%lu",
            (unsigned long)curr_read_pos_, (unsigned long)last_read_before_drops_,
            (unsigned long)drop_detection_window_,
            (unsigned long)max_drops_duration_);

    return false;
}

} // namespace audio
} // namespace roc
