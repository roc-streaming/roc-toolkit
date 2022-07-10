/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/freq_estimator.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

// Calculates dot product of arrays IR of filter (@p coeff) and input array (@p samples).
//
// - @p coeff Filter impulse response.
// - @p samples Array with sample values.
// - @p sample_ind index in input array to start from.
// - @p len How many samples do we need at output.
// - @p len_mask Bit mask of input array length.
float dot_prod(const float* coeff,
               const float* samples,
               const size_t sample_ind,
               const size_t len,
               const size_t len_mask) {
    double accum = 0;

    for (size_t i = sample_ind, j = 0; j < len; i = (i - 1) & len_mask, ++j) {
        accum += (double)coeff[j] * (double)samples[i];
    }

    return (float)accum;
}

} // namespace

FreqEstimator::FreqEstimator(FreqEstimatorConfig config,
                             packet::timestamp_t target_latency)
    : config_(config)
    , target_(target_latency)
    , dec1_ind_(0)
    , dec2_ind_(0)
    , samples_counter_(0)
    , accum_(0)
    , coeff_(1) {
    roc_panic_if_msg(config_.decimation_factor1 == 0,
                     "freq_estimator: decimation "
                     "factor 1 must not be zero");

    roc_panic_if_msg(config_.decimation_factor1 > fe_decim_factor_max
                         || config_.decimation_factor2 > fe_decim_factor_max,
                     "freq_estimator: decimation factor must be less or equal to %lu",
                     fe_decim_factor_max);

    if (fe_decim_len % 2 != 0) {
        roc_panic("freq estimator: decim_len should be power of two");
    }

    memset(dec1_casc_buff_, 0, sizeof(dec1_casc_buff_));
    memset(dec2_casc_buff_, 0, sizeof(dec2_casc_buff_));

    for (size_t i = 0; i < fe_decim_len; i++) {
        dec1_casc_buff_[i] = target_;
        dec2_casc_buff_[i] = target_;
    }
}

float FreqEstimator::freq_coeff() const {
    return coeff_;
}

void FreqEstimator::update(packet::timestamp_t current) {
    float filtered;

    if (run_decimators_(current, filtered)) {
        coeff_ = run_controller_(filtered);
    }
}

bool FreqEstimator::run_decimators_(packet::timestamp_t current, float& filtered) {
    samples_counter_++;

    dec1_casc_buff_[dec1_ind_] = (float)current;

    if ((samples_counter_ % config_.decimation_factor1) == 0) {
        // Time to calculate first decimator's samples.
        dec2_casc_buff_[dec2_ind_] = dot_prod(fe_decim_h, dec1_casc_buff_, dec1_ind_,
                                              fe_decim_len, fe_decim_len_mask)
            / fe_decim_h_gain;

        // If the second stage decimator is totally turned off
        if (config_.decimation_factor2 == 0) {
            filtered = dec2_casc_buff_[dec2_ind_];
            return true;
        } else if (((samples_counter_
                     % (config_.decimation_factor1 * config_.decimation_factor2))
                    == 0)) {
            samples_counter_ = 0;

            // Time to calculate second decimator (and freq estimator's) output.
            filtered = dot_prod(fe_decim_h, dec2_casc_buff_, dec2_ind_, fe_decim_len,
                                fe_decim_len_mask)
                / fe_decim_h_gain;

            return true;
        }

        dec2_ind_ = (dec2_ind_ + 1) & fe_decim_len_mask;
    }

    dec1_ind_ = (dec1_ind_ + 1) & fe_decim_len_mask;

    return false;
}

float FreqEstimator::run_controller_(float current) {
    const float error = (current - target_);

    accum_ = accum_ + error;
    return 1 + config_.P * error + config_.I * accum_;
}

} // namespace audio
} // namespace roc
