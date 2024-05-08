/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/profiler.h
//! @brief Profiler.

#ifndef ROC_AUDIO_PROFILER_H_
#define ROC_AUDIO_PROFILER_H_

#include "roc_audio/frame.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace audio {

//! Profiler Configuration Parameters.
//! Controls profiling interval and duration of each circular buffer chunk.
struct ProfilerConfig {
    //! Default Initialization.
    ProfilerConfig()
        : profiling_interval(core::Second)
        , chunk_duration(10 * core::Millisecond) {
    }

    //! Override Initialization.
    ProfilerConfig(core::nanoseconds_t interval, core::nanoseconds_t duration)
        : profiling_interval(interval)
        , chunk_duration(duration) {
    }

    //! Rolling window duration and reporting interval.
    core::nanoseconds_t profiling_interval;

    //! Duration of samples each chunk can hold in the circular buffer.
    core::nanoseconds_t chunk_duration;
};

//! Profiler
//! The role of the profiler is to report the average processing speed (# of samples
//! processed per time unit) during the last N seconds. We want to calculate the average
//! processing speed efficiently (with O(1) complexity, without allocations, and as
//! lightweight as possible). The problems with this are that we have variable-sized
//! frames and SMA requires fixed-size chunks. To efficiently perform this calculation a
//! ring buffer is employed. The idea behind the ring buffer is that each chunk of the
//! buffer is the average speed of 10ms worth of samples. The ring buffer is initialized
//! with fixed size (N * 1000)ms / (10ms) chunks. Within each chunk a weighted mean is
//! used to calculate the average speed during those 10ms. Each frame will contribute a
//! different number of samples to each chunk, the chunk speed is then weighted based on
//! how many samples are contributed at what frame speed. As the chunks get populated the
//! moving average is calculated. When the buffer is not entirely full the cumulative
//! moving average algorithm is used and once the buffer is full the simple moving average
//! algorithm is used.
class Profiler : public core::NonCopyable<> {
public:
    //! Initialization.
    Profiler(core::IArena& arena,
             const SampleSpec& sample_spec,
             ProfilerConfig profiler_config);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Profile frame speed.
    void add_frame(packet::stream_timestamp_t frame_duration,
                   core::nanoseconds_t elapsed);

    //! Get computed average.
    float get_moving_avg();

private:
    void update_moving_avg_(packet::stream_timestamp_t frame_duration,
                            core::nanoseconds_t elapsed);

    core::RateLimiter rate_limiter_;

    core::nanoseconds_t interval_;

    const size_t chunk_length_;
    const size_t num_chunks_;
    core::Array<float> chunks_;
    size_t first_chunk_num_;
    size_t last_chunk_num_;
    size_t last_chunk_samples_;

    float moving_avg_;
    bool buffer_full_;

    const SampleSpec sample_spec_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROFILER_H_
