/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/latency_config.h
//! @brief Latency config.

#ifndef ROC_AUDIO_LATENCY_CONFIG_H_
#define ROC_AUDIO_LATENCY_CONFIG_H_

#include "roc_audio/freq_estimator.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

//! Parameters for latency and feedback monitors.
struct LatencyConfig {
    //! FreqEstimator input.
    FreqEstimatorInput fe_input;

    //! FreqEstimator profile.
    FreqEstimatorProfile fe_profile;

    //! FreqEstimator update interval, nanoseconds.
    //! How often to run FreqEstimator and update Resampler scaling.
    core::nanoseconds_t fe_update_interval;

    //! Target latency, nanoseconds.
    core::nanoseconds_t target_latency;

    //! Maximum allowed deviation from target latency, nanoseconds.
    //! If the latency goes out of bounds, the session is terminated.
    core::nanoseconds_t latency_tolerance;

    //! Maximum allowed deviation of freq_coeff from 1.0.
    //! If the scaling goes out of bounds, it is trimmed.
    //! For example, 0.01 allows freq_coeff values in range [0.99; 1.01].
    float scaling_tolerance;

    //! Initialize.
    LatencyConfig(const core::nanoseconds_t default_target_latency)
        : fe_input(FreqEstimatorInput_Default)
        , fe_profile(FreqEstimatorProfile_Default)
        , fe_update_interval(5 * core::Millisecond)
        , target_latency(default_target_latency)
        , latency_tolerance(-1)
        , scaling_tolerance(0.005f) {
    }

    //! Automatically fill missing settings.
    void deduce_defaults(bool is_receiver);
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_CONFIG_H_
