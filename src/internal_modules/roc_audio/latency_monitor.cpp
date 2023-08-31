/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/latency_monitor.h"
#include "roc_audio/freq_estimator.h"
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
                               const audio::SampleSpec& input_sample_spec,
                               const audio::SampleSpec& output_sample_spec)
    : queue_(queue)
    , depacketizer_(depacketizer)
    , resampler_(resampler)
    , rate_limiter_(LogInterval)
    , update_interval_((packet::timestamp_t)input_sample_spec.ns_2_rtp_timestamp(
          config.fe_update_interval))
    , update_pos_(0)
    , has_update_pos_(false)
    , target_latency_(
          (packet::timestamp_t)input_sample_spec.ns_2_rtp_timestamp(target_latency))
    , min_latency_(input_sample_spec.ns_2_rtp_timestamp(config.min_latency))
    , max_latency_(input_sample_spec.ns_2_rtp_timestamp(config.max_latency))
    , max_scaling_delta_(config.max_scaling_delta)
    , input_sample_spec_(input_sample_spec)
    , output_sample_spec_(output_sample_spec)
    , valid_(false)
    , latency_(-1) {
    roc_log(LogDebug,
            "latency monitor: initializing:"
            " target_latency=%lu(%.3fms) in_rate=%lu out_rate=%lu"
            " fe_enable=%d fe_interval=%ld",
            (unsigned long)target_latency_,
            (double)input_sample_spec_.rtp_timestamp_2_ns(
                (packet::timestamp_diff_t)target_latency_)
                / core::Millisecond,
            (unsigned long)input_sample_spec_.sample_rate(),
            (unsigned long)output_sample_spec_.sample_rate(), (int)config.fe_enable,
            (long)config.fe_update_interval);

    if (target_latency < config.min_latency || target_latency > config.max_latency
        || target_latency <= 0) {
        roc_log(LogError,
                "latency monitor: invalid_config:"
                " target_latency=%ldns min_latency=%ldns max_latency=%ldns",
                (long)target_latency, (long)config.min_latency, (long)config.max_latency);
        return;
    }

    if (config.fe_enable) {
        if (config.fe_update_interval <= 0) {
            roc_log(LogError, "latency monitor: invalid config: fe_update_interval=%ld",
                    (long)config.fe_update_interval);
            return;
        }

        if (!resampler_) {
            roc_panic(
                "latency monitor: freq estimator is enabled, but resampler is null");
        }

        fe_.reset(new (fe_) FreqEstimator(
            config.fe_profile,
            (packet::timestamp_t)input_sample_spec.ns_2_rtp_timestamp(target_latency)));
        if (!fe_) {
            return;
        }

        if (!init_scaling_(input_sample_spec.sample_rate(),
                           output_sample_spec.sample_rate())) {
            return;
        }
    }

    valid_ = true;
}

bool LatencyMonitor::is_valid() const {
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

    latency_ = input_sample_spec_.rtp_timestamp_2_ns(latency);

    if (fe_) {
        if (latency < 0) {
            latency = 0;
        }
        if (!update_scaling_(pos, (packet::timestamp_t)latency)) {
            return false;
        }
    } else {
        report_latency_(latency);
    }

    return true;
}

bool LatencyMonitor::get_latency_(packet::timestamp_diff_t& latency) const {
    if (!depacketizer_.is_started()) {
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
        roc_log(
            LogDebug,
            "latency monitor: latency out of bounds: latency=%ld(%.3fms) min=%ld(%.3fms)",
            (long)latency,
            (double)input_sample_spec_.rtp_timestamp_2_ns(latency) / core::Millisecond,
            (long)min_latency_,
            (double)input_sample_spec_.rtp_timestamp_2_ns(min_latency_)
                / core::Millisecond);
        return false;
    }

    if (latency > max_latency_) {
        roc_log(
            LogDebug,
            "latency monitor: latency out of bounds: latency=%ld(%.3fms) max=%ld(%.3fms)",
            (long)latency,
            (double)input_sample_spec_.rtp_timestamp_2_ns(latency) / core::Millisecond,
            (long)max_latency_,
            (double)input_sample_spec_.rtp_timestamp_2_ns(max_latency_)
                / core::Millisecond);
        return false;
    }

    return true;
}

bool LatencyMonitor::init_scaling_(size_t input_sample_rate, size_t output_sample_rate) {
    roc_panic_if_not(resampler_);

    if (input_sample_rate == 0 || output_sample_rate == 0) {
        roc_log(LogError, "latency monitor: invalid sample rates: input=%lu output=%lu",
                (unsigned long)input_sample_rate, (unsigned long)output_sample_rate);
        return false;
    }

    if (!resampler_->set_scaling(1.0f)) {
        roc_log(LogError,
                "latency monitor: scaling factor out of bounds: input=%lu output=%lu",
                (unsigned long)input_sample_rate, (unsigned long)output_sample_rate);
        return false;
    }

    return true;
}

bool LatencyMonitor::update_scaling_(packet::timestamp_t pos,
                                     packet::timestamp_t latency) {
    roc_panic_if_not(resampler_);
    roc_panic_if_not(fe_);

    if (!has_update_pos_) {
        has_update_pos_ = true;
        update_pos_ = pos;
    }

    while (pos >= update_pos_) {
        fe_->update(latency);
        update_pos_ += update_interval_;
    }

    const float freq_coeff = fe_->freq_coeff();
    const float trimmed_coeff = trim_scaling_(freq_coeff);

    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "latency monitor:"
                " latency=%lu(%.3fms) target=%lu(%.3fms) fe=%.6f trim_fe=%.6f",
                (unsigned long)latency,
                (double)input_sample_spec_.rtp_timestamp_2_ns(
                    (packet::timestamp_diff_t)latency)
                    / core::Millisecond,
                (unsigned long)target_latency_,
                (double)input_sample_spec_.rtp_timestamp_2_ns(
                    (packet::timestamp_diff_t)target_latency_)
                    / core::Millisecond,
                (double)freq_coeff, (double)trimmed_coeff);
    }

    if (!resampler_->set_scaling(trimmed_coeff)) {
        roc_log(LogDebug,
                "latency monitor: scaling factor out of bounds: fe=%.6f trim_fe=%.6f",
                (double)freq_coeff, (double)trimmed_coeff);
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

void LatencyMonitor::report_latency_(packet::timestamp_diff_t latency) {
    if (!rate_limiter_.allow()) {
        return;
    }

    roc_log(LogDebug, "latency monitor: latency=%ld(%.3fms) target=%lu(%.3fms)",
            (long)latency,
            (double)latency_ / core::Millisecond,
            (unsigned long)target_latency_,
            (double)input_sample_spec_.rtp_timestamp_2_ns(
                (packet::timestamp_diff_t)target_latency_)
                / core::Millisecond);
}

core::nanoseconds_t LatencyMonitor::latency() const {
    return latency_;
}

} // namespace audio
} // namespace roc
