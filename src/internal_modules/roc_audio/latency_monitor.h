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
#include "roc_audio/latency_tuner.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/attributes.h"
#include "roc_core/csv_dumper.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/time.h"
#include "roc_fec/block_reader.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"
#include "roc_rtp/link_meter.h"

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

    //! Maximum allowed deviation from target latency, nanoseconds.
    //! If the latency goes out of bounds, the session is terminated.
    core::nanoseconds_t latency_tolerance;

    //! Maximum allowed deviation of freq_coeff from 1.0.
    //! If the scaling goes out of bounds, it is trimmed.
    //! For example, 0.01 allows freq_coeff values in range [0.99; 1.01].
    float scaling_tolerance;

    //! Automatically tune target latency within tolarance range so as to
    //!
    //! increase it when:
    //! * jitter grows
    //! * FEC start being sent or its block length grows
    //!
    //! or decrease it when jitter and FEC block length allows to.
    bool auto_tune_target_latency;

    LatencyMonitorConfig()
        : fe_enable(true)
        , fe_profile(FreqEstimatorProfile_Responsive)
        , fe_update_interval(5 * core::Millisecond)
        , latency_tolerance(0)
        , scaling_tolerance(0.005f)
        , auto_tune_target_latency(false) {
    }

    //! Automatically deduce FreqEstimator profile from target latency.
    void deduce_fe_profile(const core::nanoseconds_t target_latency) {
        fe_profile = target_latency < 30 * core::Millisecond
            // prefer responsive profile on low latencies, because gradual profile
            // won't do it at all
            ? FreqEstimatorProfile_Responsive
            // prefer gradual profile for higher latencies, because it can handle
            // higher network jitter
            : FreqEstimatorProfile_Gradual;
    }

    //! Automatically deduce latency_tolerance from target_latency.
    void deduce_latency_tolerance(core::nanoseconds_t target_latency) {
        // this formula returns target_latency * N, where N starts with larger
        // number and approaches 0.5 as target_latency grows
        // examples:
        //  target=1ms -> tolerance=8ms (x8)
        //  target=10ms -> tolerance=20ms (x2)
        //  target=200ms -> tolerance=200ms (x1)
        //  target=2000ms -> tolerance=1444ms (x0.722)
        if (target_latency < core::Millisecond) {
            target_latency = core::Millisecond;
        }
        latency_tolerance = core::nanoseconds_t(
            target_latency
            * (std::log((200 * core::Millisecond) * 2) / std::log(target_latency * 2)));
    }
};

//! Metrics of latency monitor.
struct LatencyMonitorMetrics {
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

    LatencyMonitorMetrics()
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
//!  - asks LatencyTuner to calculate scaling factor based on the actual and
//!    target latencies
//!  - passes calculated scaling factor to resampler
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
//!  - latency monitor has an instance of LatencyTuner; it continuously passes
//!    calculated latencies to it, and obtains scaling factor for resampler
//!  - latency monitor has a reference to resampler, and periodically passes
//!    updated scaling factor to it
//!  - pipeline also can query latency monitor for latency metrics on behalf of
//!    request from user or to report them to sender via RTCP
class LatencyMonitor : public IFrameReader, public core::NonCopyable<> {
public:
    //! Constructor.
    LatencyMonitor(IFrameReader& frame_reader,
                   const packet::SortedQueue& incoming_queue,
                   const Depacketizer& depacketizer,
                   const packet::ILinkMeter& link_meter,
                   const fec::BlockReader* fec_reader,
                   ResamplerReader* resampler,
                   const LatencyConfig& config,
                   const SampleSpec& packet_sample_spec,
                   const SampleSpec& frame_sample_spec,
                   core::CsvDumper* dumper);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Check if the stream was not aborted.
    bool is_alive() const;

    //! Get metrics.
    const LatencyMetrics& metrics() const;

    //! Read audio frame from a pipeline.
    //! @remarks
    //!  Forwards frame from underlying reader as-is.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

    //! Report playback timestamp of last frame returned by read.
    //! @remarks
    //!  Pipeline invokes this method after adding last frame to
    //!  playback buffer and knowing its playback time.
    void reclock(core::nanoseconds_t playback_timestamp);

private:
    void compute_niq_latency_();
    void compute_e2e_latency_(core::nanoseconds_t playback_timestamp);
    void query_metrics_();

    bool pre_read_();
    void post_read_(const Frame& frame);

    bool init_scaling_();
    bool update_scaling_();

    LatencyTuner tuner_;

    LatencyMetrics latency_metrics_;
    packet::LinkMetrics link_metrics_;

    IFrameReader& frame_reader_;

    const packet::SortedQueue& incoming_queue_;
    const Depacketizer& depacketizer_;
    const packet::ILinkMeter& link_meter_;
    const fec::BlockReader* fec_reader_;

    ResamplerReader* resampler_;
    const bool enable_scaling_;

    core::nanoseconds_t capture_ts_;

    const SampleSpec packet_sample_spec_;
    const SampleSpec frame_sample_spec_;

    bool alive_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_MONITOR_H_
