/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/freq_estimator.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

namespace {

// Make config from profile.
FreqEstimatorConfig make_config(FreqEstimatorProfile profile) {
    FreqEstimatorConfig config;

    switch (profile) {
    case FreqEstimatorProfile_Responsive:
        config.P = 1e-6;
        config.I = 1e-10;
        config.decimation_factor1 = fe_decim_factor_max;
        config.decimation_factor2 = 0;
        config.stable_criteria = 0.1;
        break;

    case FreqEstimatorProfile_Gradual:
        config.P = 1e-6;
        config.I = 5e-9;
        config.decimation_factor1 = fe_decim_factor_max;
        config.decimation_factor2 = fe_decim_factor_max;
        config.stable_criteria = 0.05;
        break;
    }
    config.stability_duration_criteria = 15 * core::Second;
    config.control_action_saturation_cap = 1e-2;

    return config;
}

// Calculate dot product of arrays IR of filter (coeff) and input array (samples).
//
// - coeff: Filter impulse response.
// - samples: Array with sample values.
// - sample_ind: index in input array to start from.
// - len: How many samples do we need at output.
// - len_mask: Bit mask of input array length.
double dot_prod(const double* coeff,
                const double* samples,
                const size_t sample_ind,
                const size_t len,
                const size_t len_mask) {
    double accum = 0;

    for (size_t i = sample_ind, j = 0; j < len; i = (i - 1) & len_mask, ++j) {
        accum += coeff[j] * samples[i];
    }

    return accum;
}

} // namespace

FreqEstimator::FreqEstimator(FreqEstimatorProfile profile,
                             packet::stream_timestamp_t target_latency,
                             dbgio::CsvDumper* dumper)
    : config_(make_config(profile))
    , target_(target_latency)
    , dec1_ind_(0)
    , dec2_ind_(0)
    , samples_counter_(0)
    , accum_(0)
    , coeff_(1)
    , stable_(false)
    , last_unstable_time_(core::timestamp(core::ClockMonotonic))
    , dumper_(dumper) {
    roc_log(LogDebug, "freq estimator: initializing: P=%e I=%e dc1=%lu dc2=%lu",
            config_.P, config_.I, (unsigned long)config_.decimation_factor1,
            (unsigned long)config_.decimation_factor2);

    roc_panic_if_msg(
        config_.decimation_factor1 < 1
            || config_.decimation_factor1 > fe_decim_factor_max,
        "freq estimator: invalid decimation factor 1: got=%lu expected=[1; %lu]",
        (unsigned long)config_.decimation_factor1, (unsigned long)fe_decim_factor_max);

    roc_panic_if_msg(
        config_.decimation_factor2 > fe_decim_factor_max,
        "freq estimator: invalid decimation factor 2: got=%lu expected=[0; %lu]",
        (unsigned long)config_.decimation_factor2, (unsigned long)fe_decim_factor_max);

    roc_panic_if_msg((fe_decim_len & (fe_decim_len - 1)) != 0,
                     "freq estimator: decim_len should be power of two");

    memset(dec1_casc_buff_, 0, sizeof(dec1_casc_buff_));
    memset(dec2_casc_buff_, 0, sizeof(dec2_casc_buff_));

    for (size_t i = 0; i < fe_decim_len; i++) {
        dec1_casc_buff_[i] = target_;
        dec2_casc_buff_[i] = target_;
    }
}

float FreqEstimator::freq_coeff() const {
    return (float)coeff_;
}

void FreqEstimator::update_current_latency(packet::stream_timestamp_t current_latency) {
    double filtered;

    if (run_decimators_(current_latency, filtered)) {
        if (dumper_) {
            dump_(filtered);
        }
        coeff_ = run_controller_(filtered);
    }
}

bool FreqEstimator::run_decimators_(packet::stream_timestamp_t current,
                                    double& filtered) {
    samples_counter_++;

    dec1_casc_buff_[dec1_ind_] = current;

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

double FreqEstimator::run_controller_(double current) {
    const double error = (current - target_);

    roc_log(LogTrace,
            "freq estimator:"
            " current latency error: %.0f",
            error);

    const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

    if (std::abs(error) > target_ * config_.stable_criteria && stable_) {
        stable_ = false;
        accum_ = 0;
        last_unstable_time_ = now;
        roc_log(LogDebug,
                "freq estimator: "
                " unstable, %0.f > %.0f / %0.f",
                config_.stable_criteria, error, target_);
    } else if (std::abs(error) < target_ * config_.stable_criteria && !stable_
               && now - last_unstable_time_ > config_.stability_duration_criteria) {
        stable_ = true;
        roc_log(LogDebug,
                "freq estimator: "
                " stabilized");
    }

    double res = 0.;
    // In stable state we are not using P term in order to avoid permanent variation
    // of resampler control input.
    if (stable_) {
        accum_ = accum_ + error;
        res += config_.I * accum_;
    } else {
        res += config_.P * error;
    }
    if (std::abs(res) > config_.control_action_saturation_cap) {
        res = res / std::abs(res) * config_.control_action_saturation_cap;
    }
    res += 1.;

    return res;
}

void FreqEstimator::dump_(double filtered) {
    dbgio::CsvEntry e;
    e.type = 'f';
    e.n_fields = 5;
    e.fields[0] = core::timestamp(core::ClockUnix);
    e.fields[1] = filtered;
    e.fields[2] = target_;
    e.fields[3] = (filtered - target_) * config_.P;
    e.fields[4] = accum_ * config_.I;
    dumper_->write(e);
}

void FreqEstimator::update_target_latency(packet::stream_timestamp_t target_latency) {
    target_ = (double)target_latency;
}

bool FreqEstimator::is_stable() const {
    return stable_;
}

static const char* fe_profile_to_str(FreqEstimatorProfile profile) {
    switch (profile) {
    case FreqEstimatorProfile_Responsive:
        return "responsive";

    case FreqEstimatorProfile_Gradual:
        return "gradual";
    }

    return "<invalid>";
}

} // namespace audio
} // namespace roc
