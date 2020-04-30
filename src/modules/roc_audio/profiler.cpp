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
    , interval_(interval)
    , chunk_length_(sample_rate / 100)
    , num_chunks_((size_t)(interval / (core::Second / 100)) + 1)
    , chunks_(allocator)
    , moving_avg_(0)
    , sample_rate_(sample_rate)
    , num_channels_(packet::num_channels(channels)) {
    if (num_channels_ == 0) {
        roc_panic("profiler: n_channels is zero");
    }
    if (sample_rate_ == 0) {
        roc_panic("profiler: sample_rate is zero");
    }
    chunks_.resize(num_chunks_);
}

void Profiler::begin_frame(size_t frame_size) {
    if (frame_size % num_channels_ != 0) {
        roc_panic("profiler: unexpected frame size");
    }
}

void Profiler::end_frame(size_t frame_size, core::nanoseconds_t elapsed) {
    static size_t first_chunk_num = 0;    // index of first chunk
    static size_t last_chunk_num = 0;     // index of last chunk
    static size_t last_chunk_samples = 0; // number of samples so far added to last chunk

    double frame_speed = frame_size * core::Second / (unsigned long long)elapsed;

    while (frame_size > 0) {
        size_t n_samples = frame_size;
        if (n_samples > (chunk_length_ - last_chunk_samples)) {
            n_samples = (chunk_length_ - last_chunk_samples);
        }

        double& last_chunk_speed = chunks_[last_chunk_num];
        last_chunk_samples += n_samples;
        last_chunk_speed +=
            (frame_speed - last_chunk_speed) / last_chunk_samples * n_samples;

        // last chunk is full
        if (last_chunk_samples == chunk_length_) {
            last_chunk_num = (last_chunk_num + 1) % num_chunks_;

            // ring buffer is full
            if (last_chunk_num == first_chunk_num) {
                moving_avg_ +=
                    (last_chunk_speed - chunks_[first_chunk_num]) / (num_chunks_ - 1);
                first_chunk_num = (first_chunk_num + 1) % num_chunks_;
            } else {
                moving_avg_ = ((moving_avg_ * (last_chunk_num - 1) + last_chunk_speed)
                               / last_chunk_num);
            }

            last_chunk_samples = 0;
            chunks_[last_chunk_num] = 0;
        }

        frame_size -= n_samples;
    }

    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "profiler avg for last %llu sec : %lu sample/sec (%.2f sec/sec)",
                (unsigned long long)(interval_ / core::Second),
                (unsigned long)moving_avg_, moving_avg_ / sample_rate_);
    }
}

} // roc
} // audio
