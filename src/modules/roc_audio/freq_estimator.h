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

#include "roc_config/config.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/units.h"
#include "roc_audio/freq_estimator_decim10_len.h"

namespace roc {
namespace audio {

//! Evaluates sender's frequency to receivers's frequency ratio.
class FreqEstimator : public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p aim_queue_size defines queue size we want to archive.
    FreqEstimator(
        packet::timestamp_t aim_queue_size = ROC_CONFIG_DEFAULT_SESSION_LATENCY);

    //! Compute new value of frequency coefficient.
    void update(packet::timestamp_t queue_size);

    //! Get current frequecy coefficient.
    float freq_coeff() const;

private:
    // Sample type.
    typedef packet::sample_t sample_t;

    // Calculate regulator input.
    // `in' is current queue size.
    float fast_controller_(const sample_t in);

    const sample_t aim_; // Aim queue size.

    sample_t dec1_casc_buff_[FREQ_EST_DECIM_10_LEN];
    size_t dec1_ind_;

    sample_t dec2_casc_buff_[FREQ_EST_DECIM_10_LEN];
    size_t dec2_ind_;

    size_t samples_counter_; // Input samples counter.
    sample_t accum_;         // Integrator value.

    float coeff_; // Current frequency coefficient value.
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FREQ_ESTIMATOR_H_
