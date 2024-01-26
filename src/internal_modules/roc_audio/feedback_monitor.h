/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/feedback_monitor.h
//! @brief Feedback monitor.

#ifndef ROC_AUDIO_FEEDBACK_MONITOR_H_
#define ROC_AUDIO_FEEDBACK_MONITOR_H_

#include "roc_audio/freq_estimator.h"
#include "roc_audio/iframe_writer.h"
#include "roc_audio/latency_config.h"
#include "roc_audio/resampler_writer.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Metrics for feedback monitor.
struct FeedbackMonitorMetrics {
    //! Estimated interarrival jitter.
    //! An estimate of the statistical variance of the RTP data packet
    //! interarrival time.
    core::nanoseconds_t jitter;

    //! Estimated network incoming queue latency.
    //! An estimate of how much media is buffered in receiver packet queue.
    core::nanoseconds_t niq_latency;

    //! Estimated end-to-end latency.
    //! An estimate of the time from recording a frame on sender to playing it
    //! on receiver.
    core::nanoseconds_t e2e_latency;

    FeedbackMonitorMetrics()
        : jitter(0)
        , niq_latency(0)
        , e2e_latency(0) {
    }
};

//! Feedback monitor.
class FeedbackMonitor : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Constructor.
    FeedbackMonitor(IFrameWriter& writer,
                    ResamplerWriter* resampler,
                    const SampleSpec& sample_spec,
                    const LatencyConfig& config);

    //! Check if the object was initialized successfully.
    bool is_valid() const;

    //! Check if feedback monitoring is started.
    bool is_started() const;

    //! Get metrics.
    FeedbackMonitorMetrics metrics() const;

    //! Enable feedback monitoring.
    void start();

    //! Update metrics.
    void store(const FeedbackMonitorMetrics& metrics);

    //! Write audio frame.
    virtual void write(Frame& frame);

private:
    bool update_();

    bool init_scaling_();
    bool update_scaling_(packet::stream_timestamp_diff_t latency);

    void report_();

    IFrameWriter& writer_;

    ResamplerWriter* resampler_;
    core::Optional<FreqEstimator> fe_;

    packet::stream_timestamp_t stream_pos_;

    const packet::stream_timestamp_t update_interval_;
    packet::stream_timestamp_t update_pos_;

    const packet::stream_timestamp_t report_interval_;
    packet::stream_timestamp_t report_pos_;
    bool first_report_;

    float freq_coeff_;
    const FreqEstimatorInput fe_input_;
    const float fe_max_delta_;

    packet::stream_timestamp_diff_t niq_latency_;
    packet::stream_timestamp_diff_t e2e_latency_;
    bool has_niq_latency_;
    bool has_e2e_latency_;

    packet::stream_timestamp_diff_t jitter_;
    bool has_jitter_;

    const packet::stream_timestamp_diff_t target_latency_;

    const SampleSpec sample_spec_;

    bool started_;
    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FEEDBACK_MONITOR_H_
