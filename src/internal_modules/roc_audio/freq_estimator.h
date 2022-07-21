/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/freq_estimator.h
//! @brief Frequency estimator.

#ifndef ROC_AUDIO_FREQ_ESTIMATOR_H_
#define ROC_AUDIO_FREQ_ESTIMATOR_H_

#include "roc_audio/freq_estimator_decim.h"
#include "roc_audio/sample.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! FreqEstimator tunable parameters.
struct FreqEstimatorConfig {
    float P; //!< Proportional gain of PI-controller.
    float I; //!< Integral gain of PI-controller.

    //! How much downsample input value (latency buffer size) on the first stage. Must be
    //! less or equal to fe_decim_factor_max and must be greater than zero.
    size_t decimation_factor1;
    //! How much downsample input value on the second stage. Must be
    //! less or equal to fe_decim_factor_max. Could be zero to disable the second
    //! decimation stage.
    size_t decimation_factor2;

    FreqEstimatorConfig()
        : P(100e-8f)
        , I(0.5e-8f)
        , decimation_factor1(fe_decim_factor_max)
        , decimation_factor2(fe_decim_factor_max) {
    }
};

//! Evaluates sender's frequency to receivers's frequency ratio.
class FreqEstimator : public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p target_latency defines latency we want to archive.
    explicit FreqEstimator(FreqEstimatorConfig config,
                           packet::timestamp_t target_latency);
    //! Get current frequecy coefficient.
    float freq_coeff() const;

    //! Compute new value of frequency coefficient.
    void update(packet::timestamp_t current_latency);

private:
    bool run_decimators_(packet::timestamp_t current, float& filtered);
    float run_controller_(float current);

    const FreqEstimatorConfig config_;
    const float target_; // Target latency.

    float dec1_casc_buff_[fe_decim_len];
    size_t dec1_ind_;

    float dec2_casc_buff_[fe_decim_len];
    size_t dec2_ind_;

    size_t samples_counter_; // Input samples counter.
    float accum_;            // Integrator value.

    float coeff_; // Current frequency coefficient value.
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FREQ_ESTIMATOR_H_
