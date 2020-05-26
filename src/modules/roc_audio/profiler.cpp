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
                   ProfilerConfig profiler_config)
    : rate_limiter_(profiler_config.profiling_interval)
    , interval_(profiler_config.profiling_interval)
    , chunk_length_((size_t)(
          sample_rate * ((float)profiler_config.chunk_duration / (float)core::Second)))
    , num_chunks_((size_t)((unsigned long)profiler_config.profiling_interval
                           / (unsigned long)(profiler_config.chunk_duration))
                  + 1)
    , chunks_(allocator)
    , first_chunk_num_(0)
    , last_chunk_num_(0)
    , last_chunk_samples_(0)
    , moving_avg_(0)
    , sample_rate_(sample_rate)
    , num_channels_(packet::num_channels(channels))
    , valid_(false)
    , buffer_full_(false) {
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

void Profiler::update_moving_avg_(size_t frame_size, core::nanoseconds_t elapsed) {
    const float frame_speed = float(frame_size * core::Second) / elapsed / num_channels_;

    while (frame_size > 0) {
        size_t n_samples = std::min(frame_size, (chunk_length_ - last_chunk_samples_));

        float& last_chunk_speed = chunks_[last_chunk_num_];
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
                buffer_full_ = true;
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
}

void Profiler::add_frame(size_t frame_size, core::nanoseconds_t elapsed) {
    roc_panic_if(!valid_);

    if (frame_size % num_channels_ != 0) {
        roc_panic("profiler: unexpected frame size");
    }

    update_moving_avg_(frame_size, elapsed);

    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "profiler: avg for last %.1f sec: %lu sample/sec (%.2f sec/sec)",
                (double)interval_ / core::Second, (unsigned long)get_moving_avg(),
                (double)get_moving_avg() / sample_rate_);
    }
}

float Profiler::get_moving_avg() {
    if (!buffer_full_) {
        const size_t num_samples_in_moving_avg = (chunk_length_ * last_chunk_num_);

        return (moving_avg_ * num_samples_in_moving_avg
                + chunks_[last_chunk_num_] * last_chunk_samples_)
            / (num_samples_in_moving_avg + last_chunk_samples_);
    } else {
        const size_t num_samples_in_moving_avg = (chunk_length_ * (num_chunks_ - 1));

        return (moving_avg_ * num_samples_in_moving_avg
                - chunks_[first_chunk_num_] * last_chunk_samples_
                + chunks_[last_chunk_num_] * last_chunk_samples_)
            / num_samples_in_moving_avg;
    }
}

} // roc
} // audio
