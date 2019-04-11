/*
 * Copyright (c) 2015 Roc authors
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
#include "roc_audio/units.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Evaluates sender's frequency to receivers's frequency ratio.
class FreqEstimator : public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p target_latency defines latency we want to archive.
    explicit FreqEstimator(packet::timestamp_t target_latency);

    //! Get current frequecy coefficient.
    float freq_coeff() const;

    //! Compute new value of frequency coefficient.
    void update(packet::timestamp_t current_latency);

private:
    bool run_decimators_(packet::timestamp_t current, float& filtered);
    float run_controller_(float current);

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
