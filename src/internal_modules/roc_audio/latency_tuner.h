/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/latency_tuner.h
//! @brief Latency tuner.

#ifndef ROC_AUDIO_LATENCY_TUNER_H_
#define ROC_AUDIO_LATENCY_TUNER_H_

#include "roc_audio/freq_estimator.h"
#include "roc_audio/latency_config.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/time.h"
#include "roc_dbgio/csv_dumper.h"
#include "roc_packet/ilink_meter.h"
#include "roc_packet/units.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Latency tuner.
//!
//! On receiver, LatencyMonitor computes local metrics and passes them to LatencyTuner.
//! On sender, FeedbackMonitor obtains remote metrics and passes them to LatencyTuner
//! (if sender-side latency tuning is enabled).
//!
//! If latency tuning is enabled (latency profile is not "intact"), LatencyTuner monitors
//! how close actual latency is to target latency. Assuming that the deviation is caused
//! by the clock drift between sender and receiver, it calculates scaling factor to be
//! passed to resampler to compensate the drift.
//!
//! If latency tuning is enabled and adaptive latency is used (target latency is zero),
//! LatencyTuner constantly re-calculates target latency based on current jitter and
//! other metrics.
//!
//! In addition, LatencyTuner constantly checks whether the actual latency goes out of
//! bounds, and terminates session if that happens. It may happen either when latency
//! tuning is disabled, or if it's enabled but we didn't get the job done of the
//! compensating the clock drift.
class LatencyTuner : public core::NonCopyable<> {
public:
    //! Initialize.
    LatencyTuner(const LatencyConfig& latency_config,
                 const FreqEstimatorConfig& fe_config,
                 const SampleSpec& sample_spec,
                 dbgio::CsvDumper* dumper);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Pass updated metrics to tuner.
    //! Should be called before updating stream.
    void write_metrics(const LatencyMetrics& latency_metrics,
                       const packet::LinkMetrics& link_metrics);

    //! Update stream latency and scaling.
    //!  - in adaptive latency mode, re-calculates target latency based
    //!    on jitter and other metrics
    //!  - depending on configured backend, selects which latency from
    //!    metrics to use as the actual latency
    //!  - checks if actual latency goes out of bounds and session should be
    //!    terminated; if so, returns false
    //!  - computes updated scaling based on actual latency history and
    //!    configured profile
    bool update_stream();

    //! Advance stream by given number of samples.
    //! Should be called after updating stream.
    void advance_stream(packet::stream_timestamp_t duration);

    //! If scaling has changed, returns updated value.
    //! Otherwise, returns zero.
    //! @remarks
    //!  Latency tuner expects that this scaling will be applied to the stream
    //!  resampler, so that the latency will slowly achieve target value.
    //!  Returned value is close to 1.0.
    float fetch_scaling();

private:
    bool measure_actual_latency_(packet::stream_timestamp_diff_t& result);

    bool check_actual_latency_(packet::stream_timestamp_diff_t latency);
    void compute_scaling_(packet::stream_timestamp_diff_t latency);

    void update_target_latency_(core::nanoseconds_t peak_jitter_ns,
                                core::nanoseconds_t mean_jitter_ns,
                                core::nanoseconds_t fec_block_ns);
    void try_decrease_latency_(core::nanoseconds_t estimate,
                               core::nanoseconds_t cur_tl_ns);
    void try_increase_latency_(core::nanoseconds_t estimate,
                               core::nanoseconds_t cur_tl_ns);

    void periodic_report_();
    void dump_();

    core::Optional<FreqEstimator> fe_;

    packet::stream_timestamp_t stream_pos_;

    packet::stream_timestamp_t scale_interval_;
    packet::stream_timestamp_t scale_pos_;

    packet::stream_timestamp_t report_interval_;
    packet::stream_timestamp_t report_pos_;

    bool has_new_freq_coeff_;
    float freq_coeff_;
    const float freq_coeff_max_delta_;

    const LatencyTunerBackend backend_;
    const LatencyTunerProfile profile_;

    // True if we should actively adjust clock speed using resampler scaling.
    // Either sender or receiver does it.
    const bool enable_latency_adjustment_;
    // True if we should check that deviation from target doesn't exceed limit.
    // Receiver always does it, sender does it only if latency adjustment is on sender.
    const bool enable_tolerance_checks_;
    // True if adaptive latency mode is used (target latency is zero).
    // This flag doesn't depend on who is doing latency adjustment.
    const bool latency_is_adaptive_;

    bool has_niq_latency_;
    packet::stream_timestamp_diff_t niq_latency_;
    packet::stream_timestamp_diff_t niq_stalling_;

    bool has_e2e_latency_;
    packet::stream_timestamp_diff_t e2e_latency_;

    bool has_metrics_;
    LatencyMetrics latency_metrics_;
    packet::LinkMetrics link_metrics_;

    // In fixed mode this is constant, in adaptive mode it is changing
    // from min_target_latency_ to max_target_latency_.
    packet::stream_timestamp_diff_t cur_target_latency_;

    // For adaptive mode.
    packet::stream_timestamp_diff_t min_target_latency_;
    packet::stream_timestamp_diff_t max_target_latency_;

    // For bounds checking. May be used even when tuning is disabled.
    packet::stream_timestamp_diff_t min_actual_latency_;
    packet::stream_timestamp_diff_t max_actual_latency_;
    const packet::stream_timestamp_diff_t max_stalling_;

    const SampleSpec sample_spec_;

    enum TargetLatencyState {
        TL_IDLE,
        TL_STARTING,
        TL_COOLDOWN_AFTER_INC,
        TL_COOLDOWN_AFTER_DEC
    } target_latency_state_;

    const packet::stream_timestamp_diff_t starting_timeout_;
    const packet::stream_timestamp_diff_t cooldown_dec_timeout_;
    const packet::stream_timestamp_diff_t cooldown_inc_timeout_;

    const float max_jitter_overhead_;
    const float mean_jitter_overhead_;

    packet::stream_timestamp_t last_target_latency_update_;
    const float lat_update_upper_thrsh_;
    const float lat_update_dec_step_;
    const float lat_update_inc_step_;

    core::RateLimiter last_lat_limiter_;

    dbgio::CsvDumper* dumper_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_TUNER_H_
