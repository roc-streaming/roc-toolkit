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
#include "roc_dbgio/csv_dumper.h"
#include "roc_fec/block_reader.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"

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
                   const LatencyConfig& latency_config,
                   const FreqEstimatorConfig& fe_config,
                   const SampleSpec& packet_sample_spec,
                   const SampleSpec& frame_sample_spec,
                   dbgio::CsvDumper* dumper);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

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

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_MONITOR_H_
