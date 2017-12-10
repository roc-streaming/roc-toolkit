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

Watchdog::Watchdog(IReader& reader, packet::timestamp_t timeout)
    : reader_(reader)
    , timeout_(timeout)
    , update_time_(0)
    , read_time_(0)
    , first_(true)
    , alive_(true) {
}

void Watchdog::read(Frame& frame) {
    if (!alive_) {
        return;
    }

    reader_.read(frame);

    if (frame.flags() & audio::Frame::FlagEmpty) {
        return;
    }

    read_time_ = update_time_;
}

bool Watchdog::update(packet::timestamp_t time) {
    if (!alive_) {
        return false;
    }

    if (first_) {
        read_time_ = time;
        first_ = false;
    }

    if (has_all_frames_empty_(time)) {
        alive_ = false;
        return false;
    }

    update_time_ = time;
    return true;
}

void Watchdog::check_frame_empty_(const Frame& frame) {
    if (frame.flags() & audio::Frame::FlagEmpty) {
        return;
    }

    read_time_ = update_time_;
}

bool Watchdog::has_all_frames_empty_(const packet::timestamp_t update_time) const {
    if (update_time - read_time_ < timeout_) {
        return false;
    }

    roc_log(LogInfo,
            "watchdog: timeout reached: update_time=%lu read_time=%lu timeout=%lu",
            (unsigned long)update_time, (unsigned long)read_time_,
            (unsigned long)timeout_);
    return true;
}

} // namespace audio
} // namespace roc
