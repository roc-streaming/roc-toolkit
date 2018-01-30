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
                   packet::timestamp_t timeout,
                   packet::timestamp_t drop_window_sz,
                   packet::timestamp_t max_drop_window_num)
    : reader_(reader)
    , timeout_(timeout)
    , update_time_(0)
    , read_time_(0)
    , first_(true)
    , alive_(true)
    , num_channels_(num_channels)
    , max_drop_window_num_(max_drop_window_num)
    , drop_window_sz_(drop_window_sz)
    , non_drop_pos_(0)
    , timestamp_(0)
    , total_drop_(false)
    , curr_drop_(false) {
}

void Watchdog::read(Frame& frame) {
    if (!alive_) {
        return;
    }

    reader_.read(frame);

    check_frame_empty_(frame);
    check_frame_has_dropped_packets_(frame);
}

bool Watchdog::update(packet::timestamp_t time) {
    if (!alive_) {
        return false;
    }

    if (first_) {
        read_time_ = time;
        first_ = false;
    }

    if (has_all_frames_empty_(time) || has_dropped_frames_()) {
        alive_ = false;
        return false;
    }

    update_time_ = time;
    return true;
}

bool Watchdog::has_all_frames_empty_(const packet::timestamp_t update_time) const {
    if (update_time - read_time_ < timeout_) {
        return false;
    }

    roc_log(LogInfo,
            "watchdog: timeout reached: all frames were empty:"
            " update_time=%lu read_time=%lu timeout=%lu",
            (unsigned long)update_time, (unsigned long)read_time_,
            (unsigned long)timeout_);

    return true;
}

bool Watchdog::has_dropped_frames_() const {
    if (!total_drop_) {
        return false;
    }

    roc_log(LogInfo,
            "watchdog: too many frames have dropped packets:"
            " ts=%lu non_drop_ts=%lu"
            " drop_window_sz=%lu max_drop_window_num=%lu",
            (unsigned long)timestamp_, (unsigned long)non_drop_pos_,
            (unsigned long)drop_window_sz_, (unsigned long)max_drop_window_num_);

    return true;
}

void Watchdog::check_frame_empty_(const Frame& frame) {
    if (frame.flags() & audio::Frame::FlagEmpty) {
        return;
    }

    read_time_ = update_time_;
}

void Watchdog::check_frame_has_dropped_packets_(const Frame& frame) {
    const unsigned flags = frame.flags();
    const size_t num_samples = frame.size() / num_channels_;
    const bool end_of_window =
        timestamp_ % drop_window_sz_ + num_samples >= drop_window_sz_;

    timestamp_ += num_samples;

    if ((flags & audio::Frame::FlagPacketDrops) && !(flags & audio::Frame::FlagFull)) {
        curr_drop_ = true;
    }

    if (end_of_window) {
        if (!curr_drop_) {
            non_drop_pos_ = timestamp_;
        }
        curr_drop_ = false;
    }

    if (timestamp_ - non_drop_pos_ >= max_drop_window_num_) {
        total_drop_ = true;
    }
}

} // namespace audio
} // namespace roc
