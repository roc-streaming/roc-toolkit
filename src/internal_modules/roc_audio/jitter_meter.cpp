/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/jitter_meter.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

bool JitterMeterConfig::deduce_defaults(audio::LatencyTunerProfile latency_profile) {
    if (envelope_resistance_coeff == 0) {
        if (latency_profile == audio::LatencyTunerProfile_Responsive) {
            envelope_resistance_coeff = 0.07;
        } else {
            envelope_resistance_coeff = 0.10;
        }
    }

    return true;
}

JitterMeter::JitterMeter(const JitterMeterConfig& config, core::IArena& arena)
    : config_(config)
    , jitter_window_(arena, config.jitter_window)
    , smooth_jitter_window_(arena, config.envelope_smoothing_window_len)
    , envelope_window_(arena, config.peak_quantile_window, config.peak_quantile_coeff)
    , peak_window_(arena, config.jitter_window)
    , capacitor_charge_(0)
    , capacitor_discharge_resistance_(0)
    , capacitor_discharge_iteration_(0) {
}

const JitterMetrics& JitterMeter::metrics() const {
    return metrics_;
}

void JitterMeter::update_jitter(const core::nanoseconds_t jitter) {
    // Moving average of jitter.
    jitter_window_.add(jitter);

    // Update current value of jitter envelope based on current value of jitter.
    // Envelope is computed based on smoothed jitter + a leaky peak detector.
    smooth_jitter_window_.add(jitter);
    const core::nanoseconds_t jitter_envelope =
        update_envelope_(smooth_jitter_window_.mov_max(), jitter_window_.mov_avg());

    // Quantile of envelope.
    envelope_window_.add(jitter_envelope);
    // Moving maximum of quantile of envelope.
    peak_window_.add(envelope_window_.mov_quantile());

    metrics_.mean_jitter = jitter_window_.mov_avg();
    metrics_.peak_jitter = peak_window_.mov_max();
    metrics_.curr_jitter = jitter;
    metrics_.curr_envelope = jitter_envelope;
}

// This function calculates jitter envelope using a model of a leaky peak detector.
//
// The quantile of jitter envelope is used as the value for `peak_jitter` metric.
// LatencyTuner selects target latency based on its value. We want find lowest
// possible peak jitter and target latency that are safe (don't cause disruptions).
//
// The function tries to achieve two goals:
//
//  - The quantile of envelope (e.g. 90% of values) should be above regular repeating
//    spikes, typical for wireless networks, and should ignore occasional exceptions
//    if they're not too high and not too frequent.
//
//  - The quantile of envelope should be however increased if occasional spike is
//    really high, which is often a predictor of increasing network load
//    (i.e. if spike is abnormally high, chances are that more high spikes follows).
//
// A leaky peak detector takes immediate peaks and mimicking a leakage process when
// immediate values of jitter are lower than stored one. Without it, spikes would be
// too thin to be reliably detected by quantile.
//
// Typical jitter envelope before applying capacitor:
//
//   ------------------------------------- maximum (too high)
//     |╲
//     ||          |╲        |╲
//   --||----------||--------||----------- quantile (too low)
//   __||______|╲__||__|╲____||__|╲____
//
// And after applying capacitor:
//
//     |╲_
//   --|  |_-------|╲_-------|╲----------- quantile (good)
//     |    ╲      |  ╲_     |  ╲_
//   __|     ╲_|╲__|    ╲____|    ╲____
//
core::nanoseconds_t JitterMeter::update_envelope_(const core::nanoseconds_t cur_jitter,
                                                  const core::nanoseconds_t avg_jitter) {
    // `capacitor_charge_` represents current envelope value.
    // Each step we either instantly re-charge capacitor if we see a peak, or slowly
    // discharge it until it reaches zero or we see next peek.

    if (capacitor_charge_ < cur_jitter) {
        // If current jitter is higher than capacitor charge, instantly re-charge
        // capacitor. The charge is set to the jitter value, and the resistance to
        // discharging is proportional to the value of the jitter related to average.
        //
        // Peaks that are significantly higher than average cause very slow discharging,
        // and hence have bigger impact on the envelope's quantile.
        //
        // Peaks that are not so high discharge quicker, but if they are frequent enough,
        // capacitor value is constantly re-charged and keeps high. Hence, frequent peeks
        // also have bigger impact on the envelope's quantile.
        //
        // Peaks that are neither high nor frequent have small impact on the quantile.
        capacitor_charge_ = cur_jitter;
        capacitor_discharge_resistance_ = std::pow((double)cur_jitter / avg_jitter,
                                                   config_.envelope_resistance_exponent)
            * config_.envelope_resistance_coeff;
        capacitor_discharge_iteration_ = 0;
    } else if (capacitor_charge_ > 0) {
        // No peak detected, continue discharging (exponentially).
        capacitor_charge_ =
            core::nanoseconds_t(capacitor_charge_
                                * std::exp(-capacitor_discharge_iteration_
                                           / capacitor_discharge_resistance_));
        capacitor_discharge_iteration_++;
    }

    if (capacitor_charge_ < 0) {
        // Fully discharged. Normally doesn't happen.
        capacitor_charge_ = 0;
    }

    return capacitor_charge_;
}

} // namespace audio
} // namespace roc
