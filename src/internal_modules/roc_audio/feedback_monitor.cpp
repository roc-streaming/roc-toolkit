/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/feedback_monitor.h"
#include "roc_audio/freq_estimator.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogInterval = 5 * core::Second;

double timestamp_to_ms(const SampleSpec& sample_spec,
                       packet::stream_timestamp_diff_t timestamp) {
    return (double)sample_spec.stream_timestamp_delta_2_ns(timestamp) / core::Millisecond;
}

} // namespace

FeedbackMonitor::FeedbackMonitor(IFrameWriter& writer,
                                 ResamplerWriter* resampler,
                                 const SampleSpec& sample_spec,
                                 const LatencyConfig& config)
    : writer_(writer)
    , resampler_(resampler)
    , stream_pos_(0)
    , update_interval_(
          (packet::stream_timestamp_t)sample_spec.ns_2_stream_timestamp_delta(
              config.fe_update_interval))
    , update_pos_(0)
    , report_interval_((packet::stream_timestamp_t)
                           sample_spec.ns_2_stream_timestamp_delta(LogInterval))
    , report_pos_(0)
    , first_report_(true)
    , freq_coeff_(0)
    , fe_input_(config.fe_input)
    , fe_max_delta_(config.scaling_tolerance)
    , niq_latency_(0)
    , e2e_latency_(0)
    , has_niq_latency_(false)
    , has_e2e_latency_(false)
    , jitter_(0)
    , has_jitter_(false)
    , target_latency_(fe_input_ != audio::FreqEstimatorInput_Disable
                          ? sample_spec.ns_2_stream_timestamp_delta(config.target_latency)
                          : 0)
    , sample_spec_(sample_spec)
    , started_(false)
    , valid_(false) {
    roc_log(
        LogDebug,
        "feedback monitor: initializing:"
        " target=%lu(%.3fms) fe_input=%s fe_profile=%s fe_interval=%.3fms",
        (unsigned long)target_latency_, timestamp_to_ms(sample_spec_, target_latency_),
        fe_input_to_str(config.fe_input), fe_profile_to_str(config.fe_profile),
        timestamp_to_ms(sample_spec_, (packet::stream_timestamp_diff_t)update_interval_));

    if (fe_input_ != audio::FreqEstimatorInput_Disable) {
        if (config.fe_update_interval <= 0) {
            roc_log(LogError, "feedback monitor: invalid config: fe_update_interval=%ld",
                    (long)config.fe_update_interval);
            return;
        }

        if (config.target_latency <= 0) {
            roc_log(LogError, "feedback monitor: invalid config: target_latency=%ldns",
                    (long)config.target_latency);
            return;
        }

        if (!resampler_) {
            roc_panic(
                "feedback monitor: freq estimator is enabled, but resampler is null");
        }

        fe_.reset(new (fe_) FreqEstimator(config.fe_profile,
                                          (packet::stream_timestamp_t)target_latency_));
        if (!fe_) {
            return;
        }

        if (!init_scaling_()) {
            return;
        }
    }

    valid_ = true;
}

bool FeedbackMonitor::is_valid() const {
    return valid_;
}

bool FeedbackMonitor::is_started() const {
    return started_;
}

FeedbackMonitorMetrics FeedbackMonitor::metrics() const {
    roc_panic_if(!is_valid());

    FeedbackMonitorMetrics metrics;
    metrics.jitter = sample_spec_.stream_timestamp_delta_2_ns(jitter_);
    metrics.niq_latency = sample_spec_.stream_timestamp_delta_2_ns(niq_latency_);
    metrics.e2e_latency = sample_spec_.stream_timestamp_delta_2_ns(e2e_latency_);

    return metrics;
}

void FeedbackMonitor::start() {
    roc_panic_if(!is_valid());

    if (started_) {
        return;
    }

    roc_log(LogDebug, "feedback monitor: starting");
    started_ = true;
}

void FeedbackMonitor::store(const FeedbackMonitorMetrics& metrics) {
    roc_panic_if(!is_valid());

    if (!started_) {
        return;
    }

    if (metrics.jitter != 0) {
        jitter_ = sample_spec_.ns_2_stream_timestamp_delta(metrics.jitter);
        has_jitter_ = true;
    }

    if (metrics.niq_latency != 0) {
        niq_latency_ = sample_spec_.ns_2_stream_timestamp_delta(metrics.niq_latency);
        has_niq_latency_ = true;
    }

    if (metrics.e2e_latency != 0) {
        e2e_latency_ = sample_spec_.ns_2_stream_timestamp_delta(metrics.e2e_latency);
        has_e2e_latency_ = true;
    }
}

void FeedbackMonitor::write(Frame& frame) {
    roc_panic_if(!is_valid());

    if (!update_()) {
        // TODO(gh-183): return status code
        roc_panic("feedback monitor: update failed");
    }

    writer_.write(frame);

    stream_pos_ += frame.num_samples() / sample_spec_.num_channels();

    report_();
}

bool FeedbackMonitor::update_() {
    if (!started_) {
        return true;
    }

    if (!fe_) {
        return true;
    }

    switch (fe_input_) {
    case audio::FreqEstimatorInput_NiqLatency:
        if (has_niq_latency_) {
            if (!update_scaling_(niq_latency_)) {
                return false;
            }
        }
        break;

    case audio::FreqEstimatorInput_E2eLatency:
        if (has_e2e_latency_) {
            if (!update_scaling_(e2e_latency_)) {
                return false;
            }
        }
        break;

    default:
        break;
    }

    return true;
}

bool FeedbackMonitor::init_scaling_() {
    roc_panic_if_not(resampler_);

    if (!resampler_->set_scaling(1.0f)) {
        roc_log(LogError, "feedback monitor: can't set initial scaling");
        return false;
    }

    return true;
}

bool FeedbackMonitor::update_scaling_(packet::stream_timestamp_diff_t latency) {
    roc_panic_if_not(resampler_);
    roc_panic_if_not(fe_);

    if (latency < 0) {
        latency = 0;
    }

    if (stream_pos_ < update_pos_) {
        return true;
    }

    while (stream_pos_ >= update_pos_) {
        fe_->update((packet::stream_timestamp_t)latency);
        update_pos_ += update_interval_;
    }

    freq_coeff_ = fe_->freq_coeff();
    freq_coeff_ = std::min(freq_coeff_, 1.0f + fe_max_delta_);
    freq_coeff_ = std::max(freq_coeff_, 1.0f - fe_max_delta_);

    if (!resampler_->set_scaling(freq_coeff_)) {
        roc_log(LogDebug,
                "feedback monitor: scaling factor out of bounds: fe=%.6f trim_fe=%.6f",
                (double)fe_->freq_coeff(), (double)freq_coeff_);
        return false;
    }

    return true;
}

void FeedbackMonitor::report_() {
    if (!started_) {
        return;
    }

    if (!has_e2e_latency_ && !has_niq_latency_ && !has_jitter_) {
        return;
    }

    if (first_report_) {
        roc_log(LogInfo, "feedback monitor: got first report from receiver");
        first_report_ = false;
    }

    if (stream_pos_ < report_pos_) {
        return;
    }

    while (stream_pos_ >= report_pos_) {
        report_pos_ += report_interval_;
    }

    roc_log(LogDebug,
            "feedback monitor:"
            " e2e_latency=%ld(%.3fms) niq_latency=%ld(%.3fms) target_latency=%ld(%.3fms)"
            " jitter=%ld(%.3fms)"
            " fe=%.6f trim_fe=%.6f",
            (long)e2e_latency_, timestamp_to_ms(sample_spec_, e2e_latency_),
            (long)niq_latency_, timestamp_to_ms(sample_spec_, niq_latency_),
            (long)jitter_, timestamp_to_ms(sample_spec_, jitter_), (long)target_latency_,
            timestamp_to_ms(sample_spec_, target_latency_),
            (double)(fe_ ? fe_->freq_coeff() : 0), (double)freq_coeff_);
}

} // namespace audio
} // namespace roc
