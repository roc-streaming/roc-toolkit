/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/latency_monitor.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogRate = 5000000000;

} // namespace

LatencyMonitor::LatencyMonitor(const packet::SortedQueue& queue,
                               const Depacketizer& depacketizer,
                               ResamplerReader* resampler,
                               const LatencyMonitorConfig& config,
                               packet::timestamp_t target_latency,
                               size_t input_sample_rate,
                               size_t output_sample_rate)
    : queue_(queue)
    , depacketizer_(depacketizer)
    , resampler_(resampler)
    , fe_(target_latency)
    , rate_limiter_(LogRate)
    , update_time_(0)
    , has_update_time_(false)
    , config_(config)
    , target_latency_(target_latency)
    , sample_rate_coeff_(0.f)
    , valid_(false) {
    roc_log(LogDebug,
            "latency monitor: initializing: target_latency=%lu in_rate=%lu out_rate=%lu",
            (unsigned long)target_latency_, (unsigned long)input_sample_rate,
            (unsigned long)output_sample_rate);

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

bool LatencyMonitor::update(packet::timestamp_t time) {
    packet::signed_timestamp_t latency = 0;

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
        if (!update_resampler_(time, (packet::timestamp_t)latency)) {
            return false;
        }
    } else {
        report_latency_((packet::timestamp_t)latency);
    }

    return true;
}

bool LatencyMonitor::get_latency_(packet::signed_timestamp_t& latency) const {
    if (!depacketizer_.started()) {
        return false;
    }

    const packet::timestamp_t head = depacketizer_.timestamp();

    packet::PacketPtr latest = queue_.latest();
    if (!latest) {
        return false;
    }

    const packet::timestamp_t tail = latest->end();

    latency = ROC_UNSIGNED_SUB(packet::signed_timestamp_t, tail, head);
    return true;
}

bool LatencyMonitor::check_latency_(packet::signed_timestamp_t latency) const {
    if (latency < config_.min_latency) {
        roc_log(LogDebug, "latency monitor: latency out of bounds: latency=%ld min=%ld",
                (long)latency, (long)config_.min_latency);
        return false;
    }

    if (latency > config_.max_latency) {
        roc_log(LogDebug, "latency monitor: latency out of bounds: latency=%ld max=%ld",
                (long)latency, (long)config_.max_latency);
        return false;
    }

    return true;
}

float LatencyMonitor::trim_scaling_(float freq_coeff) const {
    const float min_coeff = 1.0f - config_.max_scaling_delta;
    const float max_coeff = 1.0f + config_.max_scaling_delta;

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

    sample_rate_coeff_ = (float)input_sample_rate / output_sample_rate;

    if (!resampler_->set_scaling(sample_rate_coeff_)) {
        roc_log(LogError, "latency monitor: scaling factor out of bounds: scaling=%.5f",
                (double)sample_rate_coeff_);
        return false;
    }

    return true;
}

bool LatencyMonitor::update_resampler_(packet::timestamp_t time,
                                       packet::timestamp_t latency) {
    if (!has_update_time_) {
        has_update_time_ = true;
        update_time_ = time;
    }

    while (time >= update_time_) {
        fe_.update(latency);
        update_time_ += config_.fe_update_interval;
    }

    const float freq_coeff = fe_.freq_coeff();
    const float trimmed_coeff = trim_scaling_(freq_coeff);
    const float adjusted_coeff = sample_rate_coeff_ * trimmed_coeff;

    if (rate_limiter_.allow()) {
        roc_log(
            LogDebug,
            "latency monitor: latency=%lu target=%lu fe=%.5f trim_fe=%.5f adj_fe=%.5f",
            (unsigned long)latency, (unsigned long)target_latency_, (double)freq_coeff,
            (double)trimmed_coeff, (double)adjusted_coeff);
    }

    if (!resampler_->set_scaling(adjusted_coeff)) {
        roc_log(LogDebug,
                "latency monitor: scaling factor out of bounds: fe=%.5f adj_fe=%.5f",
                (double)freq_coeff, (double)adjusted_coeff);
        return false;
    }

    return true;
}

void LatencyMonitor::report_latency_(packet::timestamp_t latency) {
    if (rate_limiter_.allow()) {
        roc_log(LogDebug, "latency monitor: latency=%lu target=%lu",
                (unsigned long)latency, (unsigned long)target_latency_);
    }
}

} // namespace audio
} // namespace roc
