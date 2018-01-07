/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
    FreqEstimator(packet::timestamp_t target_latency);

    //! Get current frequecy coefficient.
    float freq_coeff() const;

    //! Compute new value of frequency coefficient.
    void update(packet::timestamp_t current_latency);

private:
    // Calculate regulator input.
    // `in' is current queue size.
    float fast_controller_(sample_t in);

    const sample_t target_; // Target latency.

    sample_t dec1_casc_buff_[fe_decim_len];
    size_t dec1_ind_;

    sample_t dec2_casc_buff_[fe_decim_len];
    size_t dec2_ind_;

    size_t samples_counter_; // Input samples counter.
    sample_t accum_;         // Integrator value.

    float coeff_; // Current frequency coefficient value.
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FREQ_ESTIMATOR_H_
