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

//! Latency tuner backend.
//! Defines which latency we monitor and tune to achieve target.
enum LatencyTunerBackend {
    //! Deduce best default for given settings.
    LatencyTunerBackend_Default,

    //! Latency is Network Incoming Queue length.
    //! Calculated on receiver without use of any signaling protocol.
    //! Reported back to sender via RTCP XR.
    LatencyTunerBackend_Niq,

    //! Latency is End-to-end delay.
    //! Can on receiver if RTCP XR is supported by both sides.
    //! Reported back to sender via RTCP XR.
    LatencyTunerBackend_E2e
};

//! Latency tuner profile.
//! Defines whether and how we tune latency on fly to compensate clock
//! drift and jitter.
enum LatencyTunerProfile {
    //! Deduce best default for given settings.
    LatencyTunerProfile_Default,

    //! Do not tune latency.
    LatencyTunerProfile_Intact,

    //! Fast and responsive tuning.
    //! Good for lower network latency and jitter.
    LatencyTunerProfile_Responsive,

    //! Slow and smooth tuning.
    //! Good for higher network latency and jitter.
    LatencyTunerProfile_Gradual
};

//! Latency settings.
struct LatencyConfig {
    //! Latency tuner backend to use.
    //! @remarks
    //!  Defines which latency to monitor & tune.
    LatencyTunerBackend tuner_backend;

    //! Latency tuner profile to use.
    //! @remarks
    //!  Defines how smooth is the tuning.
    LatencyTunerProfile tuner_profile;

    //! Number of packets we use to calculate sliding statistics.
    //! @remarks
    //!  We calculate jitter statistics based on this last delivered packets.
    size_t sliding_stat_window_length;

    //! Target latency.
    //! @remarks
    //!  Latency tuner will try to keep latency close to this value.
    //! @note
    //!  If zero, default value is used if possible.
    //!  Negative value is an error.
    core::nanoseconds_t target_latency;

    //! Maximum allowed deviation from target latency.
    //! @remarks
    //!  If the latency goes out of bounds, the session is terminated.
    //! @note
    //!  If zero, default value is used if possible.
    //!  Negative value is an error.
    core::nanoseconds_t latency_tolerance;

    //! Start latency.
    //! @remarks
    //!  In case of dynamic latency the tuner will start from this value.
    //! @note
    //!  This value makes sense only when target_latency is set to 0.
    core::nanoseconds_t start_latency;

    //! Minimum allowed latency.
    //! @remarks
    //!  If the latency goes out of bounds, the session is terminated.
    //! @note
    //!  If both min_latency and max_latency are zero, defaults are used.
    core::nanoseconds_t min_latency;

    //! Maximum allowed latency.
    //! @remarks
    //!  If the latency goes out of bounds, the session is terminated.
    //! @note
    //!  If both min_latency and max_latency are zero, defaults are used.
    core::nanoseconds_t max_latency;

    //! Maximum delay since last packet before queue is considered stalling.
    //! @remarks
    //!  If niq_stalling becomes larger than stalling_tolerance, latency
    //!  tolerance checks are temporary disabled.
    //! @note
    //!  If zero, default value is used if possible.
    //!  Negative value is an error.
    core::nanoseconds_t stale_tolerance;

    //! Scaling update interval.
    //! @remarks
    //!  How often to run FreqEstimator and update Resampler scaling.
    //! @note
    //!  If zero, default value is used.
    //!  Negative value is an error.
    core::nanoseconds_t scaling_interval;

    //! Maximum allowed deviation of freq_coeff from 1.0.
    //! @remarks
    //!  If the scaling goes out of bounds, it is trimmed.
    //!  For example, 0.01 allows freq_coeff values in range [0.99; 1.01].
    //! @note
    //!  If zero, default value is used.
    //!  Negative value is an error.
    float scaling_tolerance;

    //! Latency tuner decides to adjust target latency if
    //! the current value >= estimated optimal latency *
    //! latency_decrease_relative_threshold_.
    float latency_decrease_relative_threshold_;

    //! Latency tuner does not adjusts latency for  this amount of time from
    //! the very beginning.
    core::nanoseconds_t starting_timeout;

    //! Latency tuner does not adjusts latency for this amount of time from
    //! the last decreasment.
    core::nanoseconds_t cooldown_dec_timeout;

    //! Latency tuner does not adjusts latency for this amount of time from
    //! the last increasement.
    core::nanoseconds_t cooldown_inc_timeout;

    //! Latency tuner estimates an expected latency for the current jitter statistics
    //! which is then used for decision if it should engage a regulator to adjust it.
    //! estimation = MAX(max_jitter * max_jitter_overhead,
    //!                  mean_jitter * mean_jitter_overhead);
    float max_jitter_overhead;

    //! Latency tuner estimates an expected latency for the current jitter statistics
    //! which is then used for decision if it should engage a regulator to adjust it.
    //! estimation = MAX(max_jitter * max_jitter_overhead,
    //!                  mean_jitter * mean_jitter_overhead);
    float mean_jitter_overhead;

    //! Initialize.
    LatencyConfig()
        : tuner_backend(LatencyTunerBackend_Default)
        , tuner_profile(LatencyTunerProfile_Default)
        , sliding_stat_window_length(0)
        , target_latency(0)
        , latency_tolerance(0)
        , start_latency(0)
        , min_latency(0)
        , max_latency(0)
        , stale_tolerance(0)
        , scaling_interval(0)
        , scaling_tolerance(0)
        , latency_decrease_relative_threshold_(1.7f)
        , starting_timeout(5 * core::Second)
        , cooldown_dec_timeout(5 * core::Second)
        , cooldown_inc_timeout(15 * core::Second)
        , max_jitter_overhead(5 * core::Second)
        , mean_jitter_overhead(1.15f) {
    }

    //! Automatically fill missing settings.
    void deduce_defaults(core::nanoseconds_t default_target_latency, bool is_receiver);
    //! Computes latency tolerance based on requested latency value.
    //! @remarks
    //!  This formula returns target_latency * N, where N starts with larger
    //!  number and approaches 0.5 as target_latency grows.
    //!  By default we're very tolerant and allow rather big oscillations.
    //!  Examples (for multiplier = 1):
    //!   target=1ms -> tolerance=8ms (x8)
    //!   target=10ms -> tolerance=20ms (x2)
    //!   target=200ms -> tolerance=200ms (x1)
    //!   target=2000ms -> tolerance=1444ms (x0.722)
    core::nanoseconds_t calc_latency_tolerance(const core::nanoseconds_t latency,
                                               const int multiplier) const;
};

//! Latency-related metrics.
struct LatencyMetrics {
    //! Estimated network incoming queue latency.
    //! An estimate of how much media is buffered in receiver packet queue.
    core::nanoseconds_t niq_latency;

    //! Delay since last received packet.
    //! In other words, how long there were no new packets in network incoming queue.
    core::nanoseconds_t niq_stalling;

    //! Estimated end-to-end latency.
    //! An estimate of the time from recording a frame on sender to playing it
    //! on receiver.
    core::nanoseconds_t e2e_latency;

    //! Estimated FEC block duration.
    core::nanoseconds_t fec_block_duration;

    LatencyMetrics()
        : niq_latency(0)
        , niq_stalling(0)
        , e2e_latency(0)
        , fec_block_duration(0) {
    }
};

//! Latency tuner.
//!
//! On receiver, LatencyMonitor computes local metrics and passes them to LatencyTuner.
//! On sender, FeedbackMonitor obtains remote metrics and passes them to LatencyTuner.
//! In both cases, LatencyTuner processes metrics and computes scaling factor that
//! should be passed to resampler.
//!
//! Features:
//! - monitors how close actual latency and target latency are
//! - monitors whether latency goes out of bounds
//! - assuming that the difference between actual latency and target latency is
//!   caused by the clock drift between sender and receiver, calculates scaling
//!   factor for resampler to compensate it
class LatencyTuner : public core::NonCopyable<> {
public:
    //! Initialize.
    LatencyTuner(const LatencyConfig& config,
                 const SampleSpec& sample_spec,
                 dbgio::CsvDumper* dumper);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Pass updated metrics to tuner.
    //! Tuner will use new values next time when update_stream() is called.
    void write_metrics(const LatencyMetrics& latency_metrics,
                       const packet::LinkMetrics& link_metrics);

    //! Update stream latency and scaling.
    //! This method performs all actual work:
    //!  - depending on configured backend, selects which latency from
    //!    metrics to use
    //!  - check if latency goes out of bounds and session should be
    //!    terminated; if so, returns false
    //!  - computes updated scaling based on latency history and configured
    //!    profile
    bool update_stream();

    //! Advance stream by given number of samples.
    //! Should be called after updating stream.
    void advance_stream(packet::stream_timestamp_t duration);

    //! If scaling has changed, returns updated value.
    //! Otherwise, returns zero.
    //! @remarks
    //!  Latency tuner expects that this scaling will applied to the stream
    //!  resampler, so that the latency will slowly achieve target value.
    //!  Returned value is close to 1.0.
    float fetch_scaling();

private:
    bool check_bounds_(packet::stream_timestamp_diff_t latency);
    void compute_scaling_(packet::stream_timestamp_diff_t latency);
    void report_();
    // Decides if the latency should be adjusted and orders fe_ to do so if needed.
    void update_target_latency_(core::nanoseconds_t max_jitter_ns,
                                core::nanoseconds_t mean_jitter_ns,
                                core::nanoseconds_t fec_block_ns);

    core::Optional<FreqEstimator> fe_;

    packet::stream_timestamp_t stream_pos_;

    packet::stream_timestamp_diff_t scale_interval_;
    packet::stream_timestamp_t scale_pos_;

    packet::stream_timestamp_diff_t report_interval_;
    packet::stream_timestamp_t report_pos_;

    bool has_new_freq_coeff_;
    float freq_coeff_;
    const float freq_coeff_max_delta_;

    const LatencyTunerBackend backend_;
    const LatencyTunerProfile profile_;

    const bool enable_tuning_;
    const bool enable_bounds_;

    bool has_niq_latency_;
    packet::stream_timestamp_diff_t niq_latency_;
    packet::stream_timestamp_diff_t niq_stalling_;

    bool has_e2e_latency_;
    packet::stream_timestamp_diff_t e2e_latency_;

    bool has_metrics_;
    LatencyMetrics latency_metrics_;
    packet::LinkMetrics link_metrics_;

    const bool auto_tune_;
    packet::stream_timestamp_diff_t target_latency_;
    packet::stream_timestamp_diff_t min_latency_;
    packet::stream_timestamp_diff_t max_latency_;
    packet::stream_timestamp_diff_t max_stalling_;

    const SampleSpec sample_spec_;

    enum TargetLatencyState {
        TL_NONE,
        TL_STARTING,
        TL_COOLDOWN_AFTER_INC,
        TL_COOLDOWN_AFTER_DEC
    } target_latency_state_;
    const core::nanoseconds_t starting_timeout_;
    const core::nanoseconds_t cooldown_dec_timeout_;
    const core::nanoseconds_t cooldown_inc_timeout_;
    const float max_jitter_overhead_;
    const float mean_jitter_overhead_;

    core::nanoseconds_t last_target_latency_update_;
    const float lat_update_upper_thrsh_;
    const float lat_update_dec_step_;
    const float lat_update_inc_step_;

    core::RateLimiter last_lat_limiter_;

    dbgio::CsvDumper* dumper_;

    status::StatusCode init_status_;
    void try_decrease_latency_(const core::nanoseconds_t estimate,
                               const core::nanoseconds_t now,
                               const core::nanoseconds_t cur_tl_ns);
    void try_increase_latency_(const core::nanoseconds_t estimate,
                               const core::nanoseconds_t now,
                               const core::nanoseconds_t cur_tl_ns);
};

//! Get string name of latency backend.
const char* latency_tuner_backend_to_str(LatencyTunerBackend backend);

//! Get string name of latency tuner.
const char* latency_tuner_profile_to_str(LatencyTunerProfile tuner);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_TUNER_H_
