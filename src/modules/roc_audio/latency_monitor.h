/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/latency_monitor.h
//! @brief Latency monitor.

#ifndef ROC_AUDIO_LATENCY_MONITOR_H_
#define ROC_AUDIO_LATENCY_MONITOR_H_

#include "roc_audio/depacketizer.h"
#include "roc_audio/freq_estimator.h"
#include "roc_audio/resampler_reader.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/time.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Parameters for latency monitor.
struct LatencyMonitorConfig {
    //! FreqEstimator update interval, nanoseconds.
    //! How often to run FreqEstimator and update Resampler scaling.
    core::nanoseconds_t fe_update_interval;

    //! Minimum allowed latency, nanoseconds.
    //! If the latency goes out of bounds, the session is terminated.
    core::nanoseconds_t min_latency;

    //! Maximum allowed latency, nanoseconds.
    //! If the latency goes out of bounds, the session is terminated.
    core::nanoseconds_t max_latency;

    //! Maximum allowed freq_coeff delta around one.
    //! If the scaling goes out of bounds, it is trimmed.
    //! For example, 0.01 allows freq_coeff values in range [0.99; 1.01].
    float max_scaling_delta;

    LatencyMonitorConfig()
        : fe_update_interval(5 * core::Millisecond)
        , min_latency(0)
        , max_latency(0)
        , max_scaling_delta(0.005f) {
    }
};

//! Session latency monitor.
//!  - calculates session latency
//!  - calculates session scaling factor
//!  - trims scaling factor to the allowed range
//!  - updates resampler scaling
//!  - shutdowns session if the latency goes out of bounds
class LatencyMonitor : public core::NonCopyable<> {
public:
    //! Constructor.
    //!
    //! @b Parameters
    //!  - @p queue and @p depacketizer are used to calculate the latency
    //!  - @p resampler is used to set the scaling factor, may be null
    //!  - @p config defines various miscellaneous parameters
    //!  - @p target_latency defines FreqEstimator target latency, in samples
    //!  - @p input_sample_rate is the sample rate of the input packets
    //!  - @p output_sample_rate is the sample rate of the output frames
    LatencyMonitor(const packet::SortedQueue& queue,
                   const Depacketizer& depacketizer,
                   ResamplerReader* resampler,
                   const LatencyMonitorConfig& config,
                   core::nanoseconds_t target_latency,
                   size_t input_sample_rate,
                   size_t output_sample_rate);

    //! Check if the object was initialized successfully.
    bool valid() const;

    //! Update latency.
    //! @returns
    //!  false if the session should be terminated.
    bool update(packet::timestamp_t time);

private:
    bool get_latency_(packet::timestamp_diff_t& latency) const;
    bool check_latency_(packet::timestamp_diff_t latency) const;

    float trim_scaling_(float scaling) const;

    bool init_resampler_(size_t input_sample_rate, size_t output_sample_rate);
    bool update_resampler_(packet::timestamp_t time, packet::timestamp_t latency);

    void report_latency_(packet::timestamp_diff_t latency);

    const packet::SortedQueue& queue_;
    const Depacketizer& depacketizer_;
    ResamplerReader* resampler_;
    FreqEstimator fe_;

    core::RateLimiter rate_limiter_;

    const packet::timestamp_t update_interval_;
    packet::timestamp_t update_pos_;
    bool has_update_pos_;

    const packet::timestamp_t target_latency_;
    const packet::timestamp_diff_t min_latency_;
    const packet::timestamp_diff_t max_latency_;

    const float max_scaling_delta_;

    size_t input_sample_rate_;
    size_t output_sample_rate_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_MONITOR_H_
