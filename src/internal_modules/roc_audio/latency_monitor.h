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
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/time.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"
#include "roc_rtp/link_meter.h"

namespace roc {
namespace audio {

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
//!  - latency monitor has an instance of LatencyTuner; it continously passes
//!    calculated latencies to it, and obtains scaling factor for resampler
//!  - latency monitor has a reference to resampler, and periodically passes
//!    updated scaling factor to it
//!  - pipeline also can query latency monitor for latency metrics on behalf of
//!    request from user or to report them to sender via RTCP
class LatencyMonitor : public IFrameReader, public core::NonCopyable<> {
public:
    //! Constructor.
    //!
    //! @b Parameters
    //!  - @p frame_reader is inner frame reader for E2E latency calculation
    //!  - @p incoming_queue and @p depacketizer are used to NIQ latency calculation
    //!  - @p link_meter is used to obtain link metrics
    //!  - @p resampler is used to set the scaling factor to compensate clock
    //!    drift according to calculated latency
    //!  - @p config defines calculation parameters
    //!  - @p packet_sample_spec is the sample spec of the input packets
    //!  - @p frame_sample_spec is the sample spec of the output frames (after
    //!    resampling)
    LatencyMonitor(IFrameReader& frame_reader,
                   const packet::SortedQueue& incoming_queue,
                   const Depacketizer& depacketizer,
                   const rtp::LinkMeter& link_meter,
                   ResamplerReader* resampler,
                   const LatencyConfig& config,
                   const SampleSpec& packet_sample_spec,
                   const SampleSpec& frame_sample_spec);

    //! Check if the object was initialized successfully.
    bool is_valid() const;

    //! Check if the stream is still alive.
    bool is_alive() const;

    //! Get metrics.
    LatencyMetrics metrics() const;

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
    void query_link_meter_();

    bool pre_process_(const Frame& frame);
    void post_process_(const Frame& frame);

    bool init_scaling_();
    bool update_scaling_();

    LatencyTuner tuner_;
    LatencyMetrics metrics_;

    IFrameReader& frame_reader_;

    const packet::SortedQueue& incoming_queue_;
    const Depacketizer& depacketizer_;
    const rtp::LinkMeter& link_meter_;

    ResamplerReader* resampler_;
    const bool enable_scaling_;

    core::nanoseconds_t capture_ts_;

    const SampleSpec packet_sample_spec_;
    const SampleSpec frame_sample_spec_;

    bool alive_;
    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_MONITOR_H_
