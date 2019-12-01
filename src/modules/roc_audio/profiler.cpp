/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/profiler.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace audio {

Profiler::Profiler(core::IAllocator& allocator,
                   packet::channel_mask_t channels,
                   size_t sample_rate,
                   core::nanoseconds_t interval)
    : rate_limiter_(interval)
    , allocator_(allocator)
    , interval_(interval)
    , running_samples_time_(0)
    , running_samples_(0)
    , moving_avg_(0)
    , sample_rate_(sample_rate)
    , num_channels_(packet::num_channels(channels)) {
    if (num_channels_ == 0) {
        roc_panic("profiler: n_channels is zero");
    }
    if (sample_rate_ == 0) {
        roc_panic("profiler: sample_rate is zero");
    }
}

void Profiler::begin_frame(size_t frame_size) {
    if (frame_size % num_channels_ != 0) {
        roc_panic("profiler: unexpected frame size");
    }
}

void Profiler::end_frame(size_t frame_size, core::nanoseconds_t elapsed) {
    size_t samples = frame_size / num_channels_;

    core::SharedPtr<FrameNode> frame =
        new (allocator_) FrameNode(samples, elapsed, allocator_);

    running_data_.push_back(*frame);
    running_samples_ += samples;
    running_samples_time_ += elapsed;

    if (running_samples_time_ > interval_) {
        while (elapsed) {
            core::SharedPtr<FrameNode> front_frame = running_data_.front();
            if (front_frame->time < elapsed) {
                running_samples_ -= front_frame->samples;
                running_samples_time_ -= front_frame->time;
                elapsed -= front_frame->time;
                running_data_.remove(*front_frame);
            } else {
                size_t samples_to_remove = front_frame->samples * (uint64_t)elapsed
                    / (uint64_t)front_frame->time;
                front_frame->samples -= samples_to_remove;
                front_frame->time -= elapsed;
                running_samples_ -= samples_to_remove;
                running_samples_time_ -= elapsed;
                elapsed = 0;
            }
        }
    }
    moving_avg_ = running_samples_ * core::Second / (uint64_t)running_samples_time_;

    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "profiler avg for last %llu sec : %lu sample/sec (%.2f sec/sec)",
                (unsigned long long)(interval_ / core::Second),
                (unsigned long)moving_avg_, moving_avg_ / sample_rate_);
    }
}

} // roc
} // audio
