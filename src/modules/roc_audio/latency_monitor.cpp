/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/latency_monitor.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogInterval = 5 * core::Second;

} // namespace

LatencyMonitor::LatencyMonitor(const packet::SortedQueue& queue,
                               const Depacketizer& depacketizer,
                               ResamplerReader* resampler,
                               const LatencyMonitorConfig& config,
                               core::nanoseconds_t target_latency,
                               size_t input_sample_rate,
                               size_t output_sample_rate)
    : queue_(queue)
    , depacketizer_(depacketizer)
    , resampler_(resampler)
    , fe_((packet::timestamp_t)packet::timestamp_from_ns(target_latency,
                                                         input_sample_rate))
    , rate_limiter_(LogInterval)
    , update_interval_((packet::timestamp_t)packet::timestamp_from_ns(
          config.fe_update_interval, input_sample_rate))
    , update_pos_(0)
    , has_update_pos_(false)
    , target_latency_((packet::timestamp_t)packet::timestamp_from_ns(target_latency,
                                                                     input_sample_rate))
    , min_latency_(packet::timestamp_from_ns(config.min_latency, input_sample_rate))
    , max_latency_(packet::timestamp_from_ns(config.max_latency, input_sample_rate))
    , max_scaling_delta_(config.max_scaling_delta)
    , input_sample_rate_(input_sample_rate)
    , output_sample_rate_(output_sample_rate)
    , valid_(false) {
    roc_log(LogDebug,
            "latency monitor: initializing: target_latency=%lu in_rate=%lu out_rate=%lu",
            (unsigned long)target_latency_, (unsigned long)input_sample_rate,
            (unsigned long)output_sample_rate);

    if (config.fe_update_interval <= 0) {
        roc_log(LogError, "latency monitor: invalid config: fe_update_interval=%ld",
                (long)config.fe_update_interval);
        return;
    }

    if (target_latency < config.min_latency || target_latency > config.max_latency
        || target_latency <= 0) {
        roc_log(LogError,
                "latency monitor: invalid_config: "
                "target_latency=%ld min_latency=%ld max_latency=%ld",
                (long)target_latency, (long)config.min_latency, (long)config.max_latency);
        return;
    }

    if (resampler_) {
        if (!init_resampler_(input_sample_rate, output_sample_rate)) {
            return;
        }
    } else {
        if (input_sample_rate != output_sample_rate) {
            roc_log(LogError,
                    "latency monitor: input and output sample rates must be equal"
                    " when resampling is disabled: in_rate=%lu, out_rate=%lu",
                    (unsigned long)input_sample_rate, (unsigned long)output_sample_rate);
            return;
        }
    }

    valid_ = true;
}

bool LatencyMonitor::valid() const {
    return valid_;
}

bool LatencyMonitor::update(packet::timestamp_t pos) {
    packet::timestamp_diff_t latency = 0;

    if (!get_latency_(latency)) {
        return true;
    }

    if (!check_latency_(latency)) {
        return false;
    }

    if (resampler_) {
        if (latency < 0) {
            latency = 0;
        }
        if (!update_resampler_(pos, (packet::timestamp_t)latency)) {
            return false;
        }
    } else {
        report_latency_(latency);
    }

    return true;
}

bool LatencyMonitor::get_latency_(packet::timestamp_diff_t& latency) const {
    if (!depacketizer_.started()) {
        return false;
    }

    const packet::timestamp_t head = depacketizer_.timestamp();

    packet::PacketPtr latest = queue_.latest();
    if (!latest) {
        return false;
    }

    const packet::timestamp_t tail = latest->end();

    latency = packet::timestamp_diff(tail, head);
    return true;
}

bool LatencyMonitor::check_latency_(packet::timestamp_diff_t latency) const {
    if (latency < min_latency_) {
        roc_log(LogDebug, "latency monitor: latency out of bounds: latency=%ld min=%ld",
                (long)latency, (long)min_latency_);
        return false;
    }

    if (latency > max_latency_) {
        roc_log(LogDebug, "latency monitor: latency out of bounds: latency=%ld max=%ld",
                (long)latency, (long)max_latency_);
        return false;
    }

    return true;
}

float LatencyMonitor::trim_scaling_(float freq_coeff) const {
    const float min_coeff = 1.0f - max_scaling_delta_;
    const float max_coeff = 1.0f + max_scaling_delta_;

    if (freq_coeff < min_coeff) {
        return min_coeff;
    }

    if (freq_coeff > max_coeff) {
        return max_coeff;
    }

    return freq_coeff;
}

bool LatencyMonitor::init_resampler_(size_t input_sample_rate,
                                     size_t output_sample_rate) {
    if (input_sample_rate == 0 || output_sample_rate == 0) {
        roc_log(LogError, "latency monitor: invalid sample rates: input=%lu output=%lu",
                (unsigned long)input_sample_rate, (unsigned long)output_sample_rate);
        return false;
    }

    if (!resampler_->set_scaling(input_sample_rate, output_sample_rate, 1.0f)) {
        roc_log(LogError,
                "latency monitor: scaling factor out of bounds: input=%lu output=%lu",
                (unsigned long)input_sample_rate, (unsigned long)output_sample_rate);
        return false;
    }

    return true;
}

bool LatencyMonitor::update_resampler_(packet::timestamp_t pos,
                                       packet::timestamp_t latency) {
    if (!has_update_pos_) {
        has_update_pos_ = true;
        update_pos_ = pos;
    }

    while (pos >= update_pos_) {
        fe_.update(latency);
        update_pos_ += update_interval_;
    }

    const float freq_coeff = fe_.freq_coeff();
    const float trimmed_coeff = trim_scaling_(freq_coeff);

    if (rate_limiter_.allow()) {
        roc_log(LogDebug, "latency monitor: latency=%lu target=%lu fe=%.5f trim_fe=%.5f",
                (unsigned long)latency, (unsigned long)target_latency_,
                (double)freq_coeff, (double)trimmed_coeff);
    }

    if (!resampler_->set_scaling(input_sample_rate_, output_sample_rate_,
                                 trimmed_coeff)) {
        roc_log(LogDebug,
                "latency monitor: scaling factor out of bounds: fe=%.5f trim_fe=%.5f",
                (double)freq_coeff, (double)trimmed_coeff);
        return false;
    }

    return true;
}

void LatencyMonitor::report_latency_(packet::timestamp_diff_t latency) {
    if (rate_limiter_.allow()) {
        roc_log(LogDebug, "latency monitor: latency=%ld target=%lu", (long)latency,
                (unsigned long)target_latency_);
    }
}

} // namespace audio
} // namespace roc
