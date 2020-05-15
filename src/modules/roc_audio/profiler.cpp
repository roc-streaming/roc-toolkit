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

namespace {
const size_t chunk_time = 10;
}

namespace roc {
namespace audio {

Profiler::Profiler(core::IAllocator& allocator,
                   packet::channel_mask_t channels,
                   size_t sample_rate,
                   core::nanoseconds_t interval)
    : rate_limiter_(interval)
    , interval_(interval)
    , chunk_length_(chunk_time * sample_rate / (core::Second / core::Millisecond))
    , num_chunks_((size_t)((unsigned long)interval
                           / (unsigned long)(chunk_time * core::Millisecond))
                  + 1)
    , chunks_(allocator)
    , first_chunk_num_(0)
    , last_chunk_num_(0)
    , last_chunk_samples_(0)
    , moving_avg_(0)
    , sample_rate_(sample_rate)
    , num_channels_(packet::num_channels(channels)) {
    if (num_channels_ == 0) {
        roc_panic("profiler: n_channels is zero");
    }
    if (sample_rate_ == 0) {
        roc_panic("profiler: sample_rate is zero");
    }

    if (!chunks_.resize(num_chunks_)) {
        roc_log(LogError, "profiler: can't allocate chunks");
        return;
    }

    valid_ = true;
}

bool Profiler::valid() const {
    return valid_;
}

void Profiler::begin_frame(size_t frame_size) {
    roc_panic_if(!valid_);

    if (frame_size % num_channels_ != 0) {
        roc_panic("profiler: unexpected frame size");
    }
}

void Profiler::end_frame(size_t frame_size, core::nanoseconds_t elapsed) {
    roc_panic_if(!valid_);

    const double frame_speed =
        double(frame_size * core::Second) / elapsed / num_channels_;

    while (frame_size > 0) {
        size_t n_samples = std::min(frame_size, (chunk_length_ - last_chunk_samples_));

        double& last_chunk_speed = chunks_[last_chunk_num_];
        last_chunk_samples_ += n_samples;

        // Weighted mean equation
        // reference: https://fanf2.user.srcf.net/hermes/doc/antiforgery/stats.pdf
        // last_chunk_speed is µ, last_chunk_samples is W
        // frame_speed is x, n_samples is w
        last_chunk_speed +=
            (frame_speed - last_chunk_speed) / last_chunk_samples_ * n_samples;

        // last chunk is full
        if (last_chunk_samples_ == chunk_length_) {
            last_chunk_num_ = (last_chunk_num_ + 1) % num_chunks_;

            // ring buffer is full
            if (last_chunk_num_ == first_chunk_num_) {
                // Simple Moving Average: https://en.wikipedia.org/wiki/Moving_average
                moving_avg_ +=
                    (last_chunk_speed - chunks_[first_chunk_num_]) / (num_chunks_ - 1);
                first_chunk_num_ = (first_chunk_num_ + 1) % num_chunks_;
            } else {
                // Cumulative Moving Average: https://en.wikipedia.org/wiki/Moving_average
                moving_avg_ = ((moving_avg_ * (last_chunk_num_ - 1) + last_chunk_speed)
                               / last_chunk_num_);
            }

            last_chunk_samples_ = 0;
            chunks_[last_chunk_num_] = 0;
        }

        frame_size -= n_samples;
    }

    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "profiler: avg for last %.1f sec: %lu sample/sec (%.2f sec/sec)",
                (double)(interval_ / core::Second), (unsigned long)moving_avg_,
                moving_avg_ / sample_rate_);
    }
}

} // roc
} // audio
