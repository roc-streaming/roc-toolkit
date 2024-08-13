/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/latency_config.h
//! @brief Latency config.

#ifndef ROC_AUDIO_LATENCY_CONFIG_H_
#define ROC_AUDIO_LATENCY_CONFIG_H_

#include "roc_core/attributes.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Latency tuner backend.
//! Defines which latency we monitor and tune to achieve target.
enum LatencyTunerBackend {
    //! Deduce best default for given settings.
    LatencyTunerBackend_Auto,

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
//! Defines whether and how we adjust latency on fly to compensate clock
//! drift and jitter.
enum LatencyTunerProfile {
    //! Deduce best default for given settings.
    LatencyTunerProfile_Auto,

    //! Do not adjust latency.
    LatencyTunerProfile_Intact,

    //! Fast and responsive adjustment.
    //! Good for lower network latency and jitter.
    LatencyTunerProfile_Responsive,

    //! Slow and smooth adjustment.
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

    //! Target latency.
    //! @remarks
    //!  If non-zero, latency tuner enters fixed latency mode, when it tries
    //!  to keep latency as close as possible to the target value.
    //!  If zero, latency tuner will enter adaptive latency mode, when it
    //!  automatically determines best target latency.
    //! @note
    //!  Negative value is an error.
    core::nanoseconds_t target_latency;

    //! Maximum allowed deviation from target latency.
    //! @remarks
    //!  In fixed latency mode (target_latency != 0), defines maximum deviation of
    //!  current latency from target_latency.
    //!  In adaptive latency mode (target_latency == 0), defines maximum deviation of
    //!  current latency below min_latency or above max_latency.
    //! @note
    //!  If zero, default value is used.
    //!  Negative value is an error.
    core::nanoseconds_t latency_tolerance;

    //! Start latency for adaptive mode.
    //! @remarks
    //!  In adaptive latency mode (target_latency == 0), defines start value
    //!  for the target latency.
    //!  Can be used only in adaptive latency mode.
    //! @note
    //!  If zero, default value is used.
    //!  Negative value is an error.
    core::nanoseconds_t start_target_latency;

    //! Minimum latency for adaptive mode.
    //! @remarks
    //!  In adaptive latency mode (target_latency == 0), defines minimum value
    //!  for the target latency.
    //!  Can be used only in adaptive latency mode.
    //! @note
    //!  If both min_latency and max_latency are zero, defaults are used.
    core::nanoseconds_t min_target_latency;

    //! Maximum latency for adaptive mode.
    //! @remarks
    //!  In adaptive latency mode (target_latency == 0), defines maximum value
    //!  for the target latency.
    //!  Can be used only in adaptive latency mode.
    //! @note
    //!  If both min_latency and max_latency are zero, defaults are used.
    core::nanoseconds_t max_target_latency;

    //! Maximum delay since last packet before queue is considered stalling.
    //! @remarks
    //!  If niq_stalling becomes larger than stalling_tolerance, latency
    //!  tolerance checks are temporary disabled.
    //! @note
    //!  If zero, default value is used.
    //!  Negative value is an error.
    core::nanoseconds_t stale_tolerance;

    //! Scaling update interval.
    //! @remarks
    //!  How often to run FreqEstimator and update Resampler scaling.
    core::nanoseconds_t scaling_interval;

    //! Maximum allowed deviation of freq_coeff from 1.0.
    //! @remarks
    //!  If the scaling goes out of bounds, it is trimmed.
    //!  For example, 0.01 allows freq_coeff values in range [0.99; 1.01].
    float scaling_tolerance;

    //! Latency tuner decides to adjust target latency if
    //! the current value >= estimated optimal latency *
    //! latency_decrease_relative_threshold_.
    float latency_decrease_relative_threshold;

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
        : tuner_backend(LatencyTunerBackend_Auto)
        , tuner_profile(LatencyTunerProfile_Auto)
        , target_latency(0)
        , latency_tolerance(0)
        , start_target_latency(0)
        , min_target_latency(0)
        , max_target_latency(0)
        , stale_tolerance(0)
        , scaling_interval(5 * core::Millisecond)
        , scaling_tolerance(0.005f)
        , latency_decrease_relative_threshold(1.7f)
        , starting_timeout(5 * core::Second)
        , cooldown_dec_timeout(5 * core::Second)
        , cooldown_inc_timeout(15 * core::Second)
        , max_jitter_overhead(1.2f)
        , mean_jitter_overhead(3.00f) {
    }

    //! Automatically fill missing settings.
    ROC_ATTR_NODISCARD bool deduce_defaults(core::nanoseconds_t default_latency,
                                            bool is_receiver);
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
    //! An estimate of time from recording a frame on sender to playing it on receiver.
    core::nanoseconds_t e2e_latency;

    //! Estimated FEC block duration.
    //! Total duration of packets within one FEC block.
    core::nanoseconds_t fec_block_duration;

    LatencyMetrics()
        : niq_latency(0)
        , niq_stalling(0)
        , e2e_latency(0)
        , fec_block_duration(0) {
    }
};

//! Get string name of latency backend.
const char* latency_tuner_backend_to_str(LatencyTunerBackend backend);

//! Get string name of latency tuner.
const char* latency_tuner_profile_to_str(LatencyTunerProfile tuner);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_CONFIG_H_
