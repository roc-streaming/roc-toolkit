/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
#include "roc_audio/resampler.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Session latency monitor.
//!  - calculates session latency
//!  - calculates session scaling factor and passes it to resampler
//!  - shutdowns session if the latency becomes too much
//!  - shutdowns session if the scaling factor becomes too much or too low
class LatencyMonitor : public core::NonCopyable<> {
public:
    //! Constructor.
    //!
    //! @b Parameters
    //!  - @p queue and @p depacketizer are used to calculate latency
    //!  - @p resampler is used to set the scaling factor, may be null
    //!  - @p update_interval defines how often to call FreqEstimator, in samples
    //!  - @p target_latency defines FreqEstimator target latency, in samples
    //!  - @p input_sample_rate is the sample rate of the input packets
    //!  - @p output_sample_rate is the sample rate of the output frames
    LatencyMonitor(const packet::SortedQueue& queue,
                   const Depacketizer& depacketizer,
                   Resampler* resampler,
                   packet::timestamp_t update_interval,
                   packet::timestamp_t target_latency,
                   size_t input_sample_rate,
                   size_t output_sample_rate);

    //! Check if the object was initialized successfully.
    bool valid() const;

    //! Update latency.
    //! @returns
    //!  false if the session should be terminated.
    bool update(packet::timestamp_t time);

private:
    packet::timestamp_t latency_() const;

    bool init_resampler_(size_t input_sample_rate, size_t output_sample_rate);
    bool update_resampler_(packet::timestamp_t time, packet::timestamp_t latency);

    const packet::SortedQueue& queue_;
    const Depacketizer& depacketizer_;
    Resampler* resampler_;
    FreqEstimator fe_;

    core::RateLimiter rate_limiter_;

    const packet::timestamp_t update_interval_;
    packet::timestamp_t update_time_;
    bool has_update_time_;

    const packet::timestamp_t target_latency_;
    float sample_rate_coef_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_LATENCY_MONITOR_H_
