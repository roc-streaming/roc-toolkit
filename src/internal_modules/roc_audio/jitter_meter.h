/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/jitter_meter.h
//! @brief Jitter metrics calculator.

#ifndef ROC_AUDIO_JITTER_METER_H_
#define ROC_AUDIO_JITTER_METER_H_

#include "roc_audio/latency_config.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_stat/mov_aggregate.h"
#include "roc_stat/mov_quantile.h"

namespace roc {
namespace audio {

//! Jitter meter parameters.
//!
//! Mean jitter is calculated as moving average of last `jitter_window` packets.
//!
//! Peak jitter calculation is performed in several steps:
//!
//!  1. Calculate jitter envelope - a curve that outlines jitter extremes.
//!     Envelope calculation is based on a smoothing window
//!     (`envelope_smoothing_window_len`) and a peak detector with capacitor
//!     (`envelope_resistance_exponent`, `envelope_resistance_coeff`).
//!
//!  2. Calculate moving quantile of the envelope - a line above certain percentage
//!     of the envelope values across moving window (`peak_quantile_coeff`,
//!     `peak_quantile_window`).
//!
//!  3. Calculate moving maximum of the envelope's quantile across last `jitter_window`
//!     samples. This is the resulting peak jitter.
struct JitterMeterConfig {
    //! Number of packets for calculating long-term jitter sliding statistics.
    //! @remarks
    //!  Increase this value if you want slower and smoother reaction.
    //!  Peak jitter is not decreased until jitter envelope is low enough
    //!  during this window.
    //! @note
    //!  Default value is about a few minutes.
    size_t jitter_window;

    //! Number of packets in small smoothing window to calculate jitter envelope.
    //! @remarks
    //!  The larger is this value, the rougher is jitter envelope.
    //! @note
    //!  Default value is a few packets.
    size_t envelope_smoothing_window_len;

    //! Exponent coefficient of capacitor resistance used in jitter envelope.
    //! @note
    //!  Capacitor discharge resistance is (peak ^ exp) * coeff, where `peak` is
    //!  the jitter peak size relative to the average jitter, `exp` is
    //!  `envelope_resistance_exponent`, and `coeff` is `envelope_resistance_coeff`.
    //! @remarks
    //!  Increase this value to make impact to the peak jitter of high spikes much
    //!  stronger than impact of low spikes.
    double envelope_resistance_exponent;

    //! Linear coefficient of capacitor resistance used in jitter envelope.
    //! @note
    //!  Capacitor discharge resistance is (peak ^ exp) * coeff, where `peak` is
    //!  the jitter peak size relative to the average jitter, `exp` is
    //!  `envelope_resistance_exponent`, and `coeff` is `envelope_resistance_coeff`.
    //! @remarks
    //!  Increase this value to make impact to the peak jitter of frequent spikes
    //!  stronger than impact of rare spikes.
    double envelope_resistance_coeff;

    //! Number of packets for calculating envelope quantile.
    //! @remarks
    //!  This window size is used to calculate moving quantile of the envelope.
    //! @note
    //!  This value is the compromise between reaction speed to the increased
    //!  jitter and ability to distinguish rare spikes from frequent ones.
    //!  If you increase this value, we can detect and cut out more spikes that
    //!  are harmless, but we react to the relevant spikes a bit slower.
    size_t peak_quantile_window;

    //! Coefficient of envelope quantile from 0 to 1.
    //! @remarks
    //!  Defines percentage of the envelope that we want to cut out.
    //! @note
    //!  E.g. value 0.9 means that we want to draw a line that is above 90%
    //!  of all envelope values across the quantile window.
    double peak_quantile_coeff;

    JitterMeterConfig()
        : jitter_window(50000)
        , envelope_smoothing_window_len(10)
        , envelope_resistance_exponent(6)
        , envelope_resistance_coeff(0)
        , peak_quantile_window(10000)
        , peak_quantile_coeff(0.92) {
    }

    //! Automatically fill missing settings.
    ROC_ATTR_NODISCARD bool deduce_defaults(audio::LatencyTunerProfile latency_profile);
};

//! Jitter metrics.
struct JitterMetrics {
    //! Moving average of the jitter.
    core::nanoseconds_t mean_jitter;

    //! Moving peak value of the jitter.
    //! @remarks
    //!  This metric is similar to moving maximum, but excludes short rate spikes
    //!  that are considered harmless.
    core::nanoseconds_t peak_jitter;

    //! Last jitter value.
    core::nanoseconds_t curr_jitter;

    //! Last jitter envelope value.
    core::nanoseconds_t curr_envelope;

    JitterMetrics()
        : mean_jitter(0)
        , peak_jitter(0)
        , curr_jitter(0)
        , curr_envelope(0) {
    }
};

//! Jitter metrics calculator.
class JitterMeter : public core::NonCopyable<JitterMeter> {
public:
    //! Initialize.
    JitterMeter(const JitterMeterConfig& config, core::IArena& arena);

    //! Get updated jitter metrics.
    const JitterMetrics& metrics() const;

    //! Update jitter metrics based on the jitter value for newly received packet.
    void update_jitter(core::nanoseconds_t jitter);

private:
    core::nanoseconds_t update_envelope_(core::nanoseconds_t cur_jitter,
                                         core::nanoseconds_t avg_jitter);

    const JitterMeterConfig config_;

    JitterMetrics metrics_;

    stat::MovAggregate<core::nanoseconds_t> jitter_window_;
    stat::MovAggregate<core::nanoseconds_t> smooth_jitter_window_;
    stat::MovQuantile<core::nanoseconds_t> envelope_window_;
    stat::MovAggregate<core::nanoseconds_t> peak_window_;

    core::nanoseconds_t capacitor_charge_;
    double capacitor_discharge_resistance_;
    double capacitor_discharge_iteration_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_JITTER_METER_H_
