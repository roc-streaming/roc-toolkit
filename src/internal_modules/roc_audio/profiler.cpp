/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/profiler.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

Profiler::Profiler(core::IArena& arena,
                   const SampleSpec& sample_spec,
                   ProfilerConfig profiler_config)
    : rate_limiter_(profiler_config.profiling_interval)
    , interval_(profiler_config.profiling_interval)
    , chunk_length_(
          (size_t)(sample_spec.sample_rate()
                   * ((float)profiler_config.chunk_duration / (float)core::Second)))
    , num_chunks_((size_t)((unsigned long)profiler_config.profiling_interval
                           / (unsigned long)(profiler_config.chunk_duration))
                  + 1)
    , chunks_(arena)
    , first_chunk_num_(0)
    , last_chunk_num_(0)
    , last_chunk_samples_(0)
    , moving_avg_(0)
    , buffer_full_(false)
    , sample_spec_(sample_spec)
    , init_status_(status::NoStatus) {
    if (profiler_config.profiling_interval < 0 || profiler_config.chunk_duration < 0
        || chunk_length_ == 0 || num_chunks_ == 0) {
        roc_log(LogError,
                "profile: invalid config:"
                " profiling_interval=%.3fms chunk_duration=%.3fms",
                (double)profiler_config.profiling_interval / core::Millisecond,
                (double)profiler_config.chunk_duration / core::Millisecond);
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (!chunks_.resize(num_chunks_)) {
        roc_log(LogError, "profiler: can't allocate chunks");
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode Profiler::init_status() const {
    return init_status_;
}

void Profiler::add_frame(packet::stream_timestamp_t frame_duration,
                         core::nanoseconds_t elapsed) {
    roc_panic_if(init_status_ != status::StatusOK);

    update_moving_avg_(frame_duration, elapsed);

    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "profiler: avg for last %.1f sec: %lu sample/sec (%.2f sec/sec)",
                (double)interval_ / core::Second, (unsigned long)get_moving_avg(),
                (double)get_moving_avg() / sample_spec_.sample_rate());
    }
}

float Profiler::get_moving_avg() {
    roc_panic_if(init_status_ != status::StatusOK);

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

void Profiler::update_moving_avg_(packet::stream_timestamp_t frame_duration,
                                  core::nanoseconds_t elapsed) {
    const float frame_speed = float(frame_duration * core::Second) / elapsed;

    while (frame_duration > 0) {
        size_t n_samples =
            std::min((size_t)frame_duration, (chunk_length_ - last_chunk_samples_));

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

        frame_duration -= (packet::stream_timestamp_t)n_samples;
    }
}

} // namespace audio
} // namespace roc
