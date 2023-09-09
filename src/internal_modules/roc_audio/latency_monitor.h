/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/latency_monitor.h
//! @brief Latency monitor.

#ifndef ROC_AUDIO_LATENCY_MONITOR_H_
#define ROC_AUDIO_LATENCY_MONITOR_H_

#include "roc_audio/depacketizer.h"
#include "roc_audio/freq_estimator.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/time.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Parameters for latency monitor.
struct LatencyMonitorConfig {
    //! Enable FreqEstimator.
    bool fe_enable;

    //! FreqEstimator profile.
    FreqEstimatorProfile fe_profile;

    //! FreqEstimator update interval, nanoseconds.
    //! How often to run FreqEstimator and update Resampler scaling.
    core::nanoseconds_t fe_update_interval;

    //! Minimum allowed latency, nanoseconds.
    //! If the latency goes out of bounds, the session is terminated.
    core::nanoseconds_t min_latency;

    //! Maximum allowed latency, nanoseconds.
    //! If the latency goes out of bounds, the session is terminated.
    core::nanoseconds_t max_latency;

    //! Maximum allowed freq_coeff delta around one.
    //! If the scaling goes out of bounds, it is trimmed.
    //! For example, 0.01 allows freq_coeff values in range [0.99; 1.01].
    float max_scaling_delta;

    LatencyMonitorConfig()
        : fe_enable(true)
        , fe_profile(FreqEstimatorProfile_Responsive)
        , fe_update_interval(5 * core::Millisecond)
        , min_latency(0)
        , max_latency(0)
        , max_scaling_delta(0.005f) {
    }

    //! Automatically deduce FreqEstimator profile from target latency.
    void deduce_fe_profile(core::nanoseconds_t target_latency) {
        fe_profile = target_latency < 30 * core::Millisecond
            // prefer responsive profile on low latencies, because gradual profile
            // won't do it at all
            ? FreqEstimatorProfile_Responsive
            // prefer gradual profile for higher latencies, because it can handle
            // higher network jitter
            : FreqEstimatorProfile_Gradual;
    }

    //! Automatically deduce min_latency from target_latency.
    void deduce_min_latency(core::nanoseconds_t target_latency) {
        min_latency = target_latency - target_latency;
    }

    //! Automatically deduce max_latency from target_latency.
    void deduce_max_latency(core::nanoseconds_t target_latency) {
        max_latency = target_latency + target_latency;
    }
};

//! Statistics of latency monitor.
struct LatencyMonitorStats {
    //! Estimated NIQ latency.
    //! NIQ = network incoming queue.
    //! Defines how many samples are buffered in receiver packet queue and
    //! receiver pipeline before depacketizer (packet part of pipeline).
    core::nanoseconds_t niq_latency;

    //! Estimated E2E latency.
    //! E2E = end-to-end.
    //! Defines how much time passed between frame entered sender pipeline
    //! (when it is captured) and leaved received pipeline (when it is played).
    core::nanoseconds_t e2e_latency;

    LatencyMonitorStats()
        : niq_latency(0)
        , e2e_latency(0) {
    }
};

//! Latency monitor.
//!
//! @b Features
//!
//!  - calculates NIQ latency (network incoming queue) - how many samples are
//!    buffered in receiver packet queue and receiver pipeline before depacketizer
//!  - calculates E2E latency (end-to-end) - how much time passed between frame
//!    was captured on sender and played on receiver (this is based on capture
//!    timestamps, which are populated with the help of RTCP)
//!  - monitors how close actual latency and target latency are (which one to
//!    check, NIQ or E2E, depends on config)
//!  - assuming that the difference between actual latency and target latency is
//!    caused by the clock drift between sender and receiver, calculates scaling
//!    factor for resampler to compensate that clock drift
//!  - passes calculated scaling factor to resampler
//!  - shuts down session if the actual latency goes out of bounds
//!
//! @b Flow
//!
//!  - pipeline periodically calls read() method; it uses references to incoming
//!    packet queue (start of the pipeline) and depacketizer (last pipeline element
//!    that works with packets), asks them about current packet / frame, and calculates
//!    distance between them, which is NIQ latency
//!  - after adding frame to playback buffer, pipeline invokes reclock() method;
//!    it calculates difference between capture and playback time of the frame,
//!    which is E2E latency
//!  - latency monitor has an instance of FreqEstimator (FE); it continously passes
//!    calculated latency to FE, and FE calculates scaling factor for resampler
//!  - latency monitor has a reference to resampler, and periodically passes
//!    updated scaling factor to it;
//!  - pipeline also can query latency monitor for latency statistics on behalf of
//!    request from user
class LatencyMonitor : public IFrameReader, public core::NonCopyable<> {
public:
    //! Constructor.
    //!
    //! @b Parameters
    //!  - @p frame_reader is inner frame reader for E2E latency calculation
    //!  - @p incoming_queue and @p depacketizer are used to NIQ latency calculation
    //!  - @p resampler is used to set the scaling factor to compensate clock
    //!    drift according to calculated latency
    //!  - @p config defines calculation parameters
    //!  - @p target_latency defines target latency that we should maintain
    //!  - @p input_sample_spec is the sample spec of the input packets
    //!  - @p output_sample_spec is the sample spec of the output frames (after
    //!    resampling)
    LatencyMonitor(IFrameReader& frame_reader,
                   const packet::SortedQueue& incoming_queue,
                   const Depacketizer& depacketizer,
                   ResamplerReader* resampler,
                   const LatencyMonitorConfig& config,
                   core::nanoseconds_t target_latency,
                   const SampleSpec& input_sample_spec,
                   const SampleSpec& output_sample_spec);

    //! Check if the object was initialized successfully.
    bool is_valid() const;

    //! Check if the stream is still alive.
    bool is_alive() const;

    //! Get statistics.
    LatencyMonitorStats stats() const;

    //! Read audio frame from a pipeline.
    //! @remarks
    //!  Forwards frame from underlying reader as-is.
    virtual bool read(Frame& frame);

    //! Report playback timestamp of last frame returned by read.
    //! @remarks
    //!  Pipeline invokes this method after adding last frame to
    //!  playback buffer and knowing its playback time.
    //! @returns
    //!  false if the session is ended
    bool reclock(core::nanoseconds_t playback_timestamp);

private:
    void compute_niq_latency_();
    void compute_e2e_latency_(core::nanoseconds_t playback_timestamp);

    bool update_();

    bool check_bounds_(packet::timestamp_diff_t latency) const;

    bool init_scaling_(size_t input_sample_rate, size_t output_sample_rate);
    bool update_scaling_(packet::timestamp_diff_t latency);

    void report_();

    IFrameReader& frame_reader_;

    const packet::SortedQueue& incoming_queue_;
    const Depacketizer& depacketizer_;

    ResamplerReader* resampler_;
    core::Optional<FreqEstimator> fe_;

    packet::timestamp_t stream_pos_;
    core::nanoseconds_t stream_cts_;

    const packet::timestamp_t update_interval_;
    packet::timestamp_t update_pos_;

    const packet::timestamp_t report_interval_;
    packet::timestamp_t report_pos_;

    float freq_coeff_;

    packet::timestamp_diff_t niq_latency_;
    packet::timestamp_diff_t e2e_latency_;
    bool has_niq_latency_;
    bool has_e2e_latency_;

    const packet::timestamp_diff_t target_latency_;
    const packet::timestamp_diff_t min_latency_;
    const packet::timestamp_diff_t max_latency_;

    const float max_scaling_delta_;

    const SampleSpec input_sample_spec_;
    const SampleSpec output_sample_spec_;

    bool alive_;
    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_MONITOR_H_
