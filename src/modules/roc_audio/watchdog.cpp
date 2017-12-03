/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/watchdog.h"
#include "roc_core/log.h"

namespace roc {
namespace audio {

Watchdog::Watchdog(IReader& reader, packet::timestamp_t timeout, size_t skip_window_sz)
    : reader_(reader)
    , timeout_(timeout)
    , update_time_(0)
    , read_time_(0)
    , first_(true)
    , alive_(true)
    , skip_time_(0)
    , frame_num_(0)
    , skip_window_sz_(skip_window_sz)
    , total_skip_(false)
    , curr_skip_(false) {
}

void Watchdog::read(Frame& frame) {
    frame_num_++;

    if (!alive_) {
        return;
    }

    reader_.read(frame);

    if (frame.flags() & audio::Frame::FlagEmpty) {
        return;
    }

    read_time_ = update_time_;

    if (frame.flags() & audio::Frame::FlagSkip) {
        if (!curr_skip_ && !total_skip_) {
            skip_time_ = update_time_;
        }
        curr_skip_ = true;
    }

    if (frame_num_ % skip_window_sz_ == 0) {
        total_skip_ = curr_skip_;
        curr_skip_ = false;
    }
}

bool Watchdog::update(packet::timestamp_t time) {
    if (!alive_) {
        return false;
    }

    if (first_) {
        read_time_ = time;
        first_ = false;
    }

    update_time_ = time;

    if (update_time_ - read_time_ >= timeout_) {
        roc_log(LogInfo,
                "watchdog: timeout reached: update_time=%lu read_time=%lu timeout=%lu",
                (unsigned long)update_time_, (unsigned long)read_time_,
                (unsigned long)timeout_);
        alive_ = false;
        return false;
    }

    if (time - skip_time_ >= timeout_) {
        if (total_skip_ || curr_skip_) {
            roc_log(LogInfo,
                    "watchdog: timeout reached: too many dropped packets:"
                    " skip_time=%lu timeout=%lu",
                    (unsigned long)skip_time_, (unsigned long)timeout_);
            alive_ = false;
            return false;
        }
        skip_time_ = time;
    }

    return true;
}

} // namespace audio
} // namespace roc
