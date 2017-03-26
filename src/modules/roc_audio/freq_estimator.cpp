/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/freq_estimator.h"
#include "roc_audio/freq_estimator_decim10_coeff.h"

namespace roc {
namespace audio {

using packet::sample_t;

namespace {

const size_t G_decim_len = FREQ_EST_DECIM_10_LEN;

const size_t G_decim_len_mask = FREQ_EST_DECIM_10_LEN_MASK;

const uint32_t G_decim_factor = 10;

// Proportional gain of PI-controller.
const sample_t G_p = 100e-8f;

// Integral gain of PI-controller.
const sample_t G_i = 0.5e-8f;

// Calculates dot product of arrays IR of filter (@p coeff) and input array (@p samples).
//
// - @p coeff Filter impulse response.
// - @p samples Array with sample values.
// - @p sample_ind index in input array to start from.
// - @p len How many samples do we need at output.
// - @p len_mask Bit mask of input array length.
sample_t dot_prod(const sample_t* coeff,
                  const sample_t* samples,
                  const size_t sample_ind,
                  const size_t len,
                  const size_t len_mask) {
    double accum = 0;

    for (size_t i = sample_ind, j = 0; j < len; i = (i - 1) & len_mask, ++j) {
        accum += (double)coeff[j] * (double)samples[i];
    }

    return (sample_t)accum;
}

} // namespace

FreqEstimator::FreqEstimator(packet::timestamp_t aim_queue_size)
    : aim_(aim_queue_size)
    , dec1_ind_(0)
    , dec2_ind_(0)
    , samples_counter_(0)
    , accum_(0)
    , coeff_(1) {
    for (size_t i = 0; i < G_decim_len; i++) {
        dec1_casc_buff_[i] = aim_;
        dec2_casc_buff_[i] = aim_;
    }
}

float FreqEstimator::freq_coeff() const {
    return coeff_;
}

void FreqEstimator::update(packet::timestamp_t queue_size) {
    samples_counter_++;

    dec1_casc_buff_[dec1_ind_] = (sample_t)queue_size;

    if ((samples_counter_ % G_decim_factor) == 0) {
        /*
         * Time to calculate first decimator's samples.
         */
        dec2_casc_buff_[dec2_ind_] =
            dot_prod(decim_10_h, dec1_casc_buff_, dec1_ind_, G_decim_len,
                     G_decim_len_mask) / decim_10_h_gain;

        if (((samples_counter_ % (G_decim_factor * G_decim_factor)) == 0)) {
            samples_counter_ = 0;
            /*
             * Time to calculate second decimator (and freq estimator's) output.
             */
            sample_t filtered_queue_len =
                dot_prod(decim_10_h, dec2_casc_buff_, dec2_ind_, G_decim_len,
                         G_decim_len_mask) / decim_10_h_gain;

            coeff_ = fast_controller_(filtered_queue_len);
        }

        dec2_ind_ = (dec2_ind_ + 1) & G_decim_len_mask;
    }

    dec1_ind_ = (dec1_ind_ + 1) & G_decim_len_mask;
}

float FreqEstimator::fast_controller_(const sample_t input) {
    accum_ = accum_ + input - aim_;
    return 1 + G_p * (input - aim_) + G_i * accum_;
}

} // namespace audio
} // namespace roc
