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

bool FreqEstimatorConfig::deduce_defaults(LatencyTunerProfile latency_profile) {
    switch (latency_profile) {
    case LatencyTunerProfile_Gradual:
        if (P == 0 && I == 0) {
            P = 1e-6;
            I = 5e-9;
        }
        if (decimation_factor1 == 0 && decimation_factor2 == 0) {
            decimation_factor1 = fe_decim_factor_max;
            decimation_factor2 = fe_decim_factor_max;
        }
        if (stable_criteria == 0) {
            stable_criteria = 0.05;
        }
        break;

    case LatencyTunerProfile_Responsive:
        if (P == 0 && I == 0) {
            P = 1e-6;
            I = 1e-10;
        }
        if (decimation_factor1 == 0 && decimation_factor2 == 0) {
            decimation_factor1 = fe_decim_factor_max;
            decimation_factor2 = 0;
        }
        if (stable_criteria == 0) {
            stable_criteria = 0.1;
        }
        break;

    case LatencyTunerProfile_Intact:
        break;

    default:
        roc_log(LogError, "freq estimator: unexpected latency tuner profile %s",
                latency_tuner_profile_to_str(latency_profile));
        return false;
    }

    return true;
}

FreqEstimator::FreqEstimator(const FreqEstimatorConfig& config,
                             packet::stream_timestamp_t target_latency,
                             const SampleSpec& sample_spec,
                             dbgio::CsvDumper* dumper)
    : config_(config)
    , dec1_ind_(0)
    , dec2_ind_(0)
    , samples_counter_(0)
    , accum_(0)
    , target_(target_latency)
    , coeff_(1)
    , stable_(false)
    , last_unstable_time_(0)
    , stability_duration_criteria_(
          sample_spec.ns_2_stream_timestamp_delta(config.stability_duration_criteria))
    , current_stream_pos_(0)
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

bool FreqEstimator::is_stable() const {
    return stable_;
}

void FreqEstimator::update_current_latency(packet::stream_timestamp_t current_latency) {
    double filtered = 0;

    if (run_decimators_(current_latency, filtered)) {
        if (dumper_) {
            dump_(filtered);
        }
        coeff_ = run_controller_(filtered);
    }
}

void FreqEstimator::update_target_latency(packet::stream_timestamp_t target_latency) {
    target_ = (double)target_latency;
}

void FreqEstimator::update_stream_position(packet::stream_timestamp_t stream_position) {
    roc_panic_if_msg(!packet::stream_timestamp_ge(stream_position, current_stream_pos_),
                     "freq estimator: expected monotonic stream position");

    current_stream_pos_ = stream_position;
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

    if (std::abs(error) > target_ * config_.stable_criteria && stable_) {
        stable_ = false;
        accum_ = 0;
        last_unstable_time_ = current_stream_pos_;
        roc_log(LogDebug,
                "freq estimator:"
                " unstable, %0.f > %.0f / %0.f",
                config_.stable_criteria, error, target_);
    } else if (std::abs(error) < target_ * config_.stable_criteria && !stable_
               && packet::stream_timestamp_diff(current_stream_pos_, last_unstable_time_)
                   > stability_duration_criteria_) {
        stable_ = true;
        roc_log(LogDebug,
                "freq estimator:"
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

} // namespace audio
} // namespace roc
