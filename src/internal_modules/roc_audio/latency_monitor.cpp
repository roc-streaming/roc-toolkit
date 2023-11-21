/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/latency_monitor.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogInterval = 5 * core::Second;

double timestamp_to_ms(const SampleSpec& sample_spec,
                       packet::stream_timestamp_diff_t timestamp) {
    return (double)sample_spec.stream_timestamp_delta_2_ns(timestamp) / core::Millisecond;
}

} // namespace

LatencyMonitor::LatencyMonitor(IFrameReader& frame_reader,
                               const packet::SortedQueue& incoming_queue,
                               const Depacketizer& depacketizer,
                               ResamplerReader* resampler,
                               const LatencyMonitorConfig& config,
                               core::nanoseconds_t target_latency,
                               const SampleSpec& input_sample_spec,
                               const SampleSpec& output_sample_spec)
    : frame_reader_(frame_reader)
    , incoming_queue_(incoming_queue)
    , depacketizer_(depacketizer)
    , resampler_(resampler)
    , stream_pos_(0)
    , stream_cts_(0)
    , update_interval_(
          (packet::stream_timestamp_t)input_sample_spec.ns_2_stream_timestamp_delta(
              config.fe_update_interval))
    , update_pos_(0)
    , report_interval_((packet::stream_timestamp_t)
                           input_sample_spec.ns_2_stream_timestamp_delta(LogInterval))
    , report_pos_(0)
    , freq_coeff_(0)
    , niq_latency_(0)
    , e2e_latency_(0)
    , has_niq_latency_(false)
    , has_e2e_latency_(false)
    , target_latency_(input_sample_spec.ns_2_stream_timestamp_delta(target_latency))
    , min_latency_(input_sample_spec.ns_2_stream_timestamp_delta(
          target_latency - config.latency_tolerance))
    , max_latency_(input_sample_spec.ns_2_stream_timestamp_delta(
          target_latency + config.latency_tolerance))
    , max_scaling_delta_(config.scaling_tolerance)
    , input_sample_spec_(input_sample_spec)
    , output_sample_spec_(output_sample_spec)
    , alive_(true)
    , valid_(false) {
    roc_log(
        LogDebug,
        "latency monitor: initializing:"
        " target=%lu(%.3fms) min=%lu(%.3fms) max=%lu(%.3fms)"
        " in_rate=%lu out_rate=%lu"
        " fe_enable=%d fe_profile=%s fe_interval=%.3fms",
        (unsigned long)target_latency_,
        timestamp_to_ms(input_sample_spec_, target_latency_), (unsigned long)min_latency_,
        timestamp_to_ms(input_sample_spec_, min_latency_), (unsigned long)max_latency_,
        timestamp_to_ms(input_sample_spec_, max_latency_),
        (unsigned long)input_sample_spec_.sample_rate(),
        (unsigned long)output_sample_spec_.sample_rate(), (int)config.fe_enable,
        fe_profile_to_str(config.fe_profile),
        timestamp_to_ms(input_sample_spec_,
                        (packet::stream_timestamp_diff_t)update_interval_));

    if (target_latency_ < min_latency_ || target_latency_ > max_latency_
        || target_latency <= 0) {
        roc_log(LogError,
                "latency monitor: invalid config:"
                " target_latency=%ldns latency_tolerance=%ldns",
                (long)target_latency, (long)config.latency_tolerance);
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

        fe_.reset(new (fe_) FreqEstimator(config.fe_profile,
                                          (packet::stream_timestamp_t)target_latency_));
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

bool LatencyMonitor::is_alive() const {
    roc_panic_if(!is_valid());

    return alive_;
}

LatencyMonitorMetrics LatencyMonitor::metrics() const {
    roc_panic_if(!is_valid());

    LatencyMonitorMetrics metrics;
    metrics.niq_latency = input_sample_spec_.stream_timestamp_delta_2_ns(niq_latency_);
    metrics.e2e_latency = input_sample_spec_.stream_timestamp_delta_2_ns(e2e_latency_);

    return metrics;
}

bool LatencyMonitor::read(Frame& frame) {
    roc_panic_if(!is_valid());

    if (frame.num_samples() % input_sample_spec_.num_channels() != 0) {
        roc_panic("latency monitor: unexpected frame size");
    }

    compute_niq_latency_();

    update_();

    if (!frame_reader_.read(frame)) {
        return false;
    }

    stream_pos_ += frame.num_samples() / input_sample_spec_.num_channels();
    stream_cts_ = frame.capture_timestamp();

    report_();

    return true;
}

bool LatencyMonitor::reclock(const core::nanoseconds_t playback_timestamp) {
    roc_panic_if(!is_valid());

    if (playback_timestamp < 0) {
        roc_panic("latency monitor: unexpected playback timestamp");
    }

    // this method is called when playback time of last frame was reported
    // now we can update e2e latency based on it
    compute_e2e_latency_(playback_timestamp);

    return true;
}

void LatencyMonitor::compute_niq_latency_() {
    if (!depacketizer_.is_started()) {
        return;
    }

    // timestamp of next sample that depacketizer expects from packet pipeline
    const packet::stream_timestamp_t niq_head = depacketizer_.next_timestamp();

    packet::PacketPtr latest_packet = incoming_queue_.latest();
    if (!latest_packet) {
        return;
    }

    // timestamp of last sample of last packet in packet pipeline
    const packet::stream_timestamp_t niq_tail = latest_packet->end();

    // packet pipeline length
    // includes incoming queue and packets buffered inside other packet
    // pipeline elements, e.g. in FEC reader
    niq_latency_ = packet::stream_timestamp_diff(niq_tail, niq_head);
    has_niq_latency_ = true;
}

void LatencyMonitor::compute_e2e_latency_(const core::nanoseconds_t playback_timestamp) {
    if (stream_cts_ == 0) {
        return;
    }

    // delta between time when first sample of last frame is played on receiver and
    // time when first sample of that frame was captured on sender
    // (both timestamps are in receiver clock domain)
    e2e_latency_ =
        input_sample_spec_.ns_2_stream_timestamp_delta(playback_timestamp - stream_cts_);
    has_e2e_latency_ = true;
}

bool LatencyMonitor::update_() {
    if (!alive_) {
        return false;
    }

    // currently scaling is always updated based on niq latency
    if (has_niq_latency_) {
        if (!check_bounds_(niq_latency_)) {
            alive_ = false;
            return false;
        }
        if (fe_) {
            if (!update_scaling_(niq_latency_)) {
                alive_ = false;
                return false;
            }
        }
    }

    return true;
}

bool LatencyMonitor::check_bounds_(const packet::stream_timestamp_diff_t latency) const {
    if (latency < min_latency_ && incoming_queue_.size() == 0) {
        // Sharp latency decrease often appears on burst packet delays or drops,
        // when depacketizer goes quite ahead of the last retrieved packet, and there
        // are no new packets. If such burst is short, pipeline can easily recover from
        // it, and terminating session would only harm. Hence, while the queue is empty,
        // latency monitor does not terminate the session and leaves decision to the
        // watchdog. If the burst was short, watchdog will keep session, otherwise
        // no_playback_timeout will trigger and watchdog will terminate session.
        return true;
    }

    if (latency < min_latency_ || latency > max_latency_) {
        roc_log(LogDebug,
                "latency monitor: latency out of bounds:"
                " latency=%ld(%.3fms) target=%ld(%.3fms)"
                " min=%ld(%.3fms) max=%ld(%.3fms) q_size=%lu",
                (long)latency, timestamp_to_ms(input_sample_spec_, latency),
                (long)target_latency_,
                timestamp_to_ms(input_sample_spec_, target_latency_), (long)min_latency_,
                timestamp_to_ms(input_sample_spec_, min_latency_), (long)max_latency_,
                timestamp_to_ms(input_sample_spec_, max_latency_),
                (unsigned long)incoming_queue_.size());
        return false;
    }

    return true;
}

bool LatencyMonitor::init_scaling_(const size_t input_sample_rate,
                                   const size_t output_sample_rate) {
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

bool LatencyMonitor::update_scaling_(packet::stream_timestamp_diff_t latency) {
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
    freq_coeff_ = std::min(freq_coeff_, 1.0f + max_scaling_delta_);
    freq_coeff_ = std::max(freq_coeff_, 1.0f - max_scaling_delta_);

    if (!resampler_->set_scaling(freq_coeff_)) {
        roc_log(LogDebug,
                "latency monitor: scaling factor out of bounds: fe=%.6f trim_fe=%.6f",
                (double)fe_->freq_coeff(), (double)freq_coeff_);
        return false;
    }

    return true;
}

void LatencyMonitor::report_() {
    if (!has_niq_latency_) {
        return;
    }

    if (stream_pos_ < report_pos_) {
        return;
    }

    while (stream_pos_ >= report_pos_) {
        report_pos_ += report_interval_;
    }

    roc_log(LogDebug,
            "latency monitor:"
            " e2e_latency=%ld(%.3fms) niq_latency=%ld(%.3fms) target_latency=%ld(%.3fms)"
            " fe=%.6f trim_fe=%.6f",
            (long)e2e_latency_, timestamp_to_ms(input_sample_spec_, e2e_latency_),
            (long)niq_latency_, timestamp_to_ms(input_sample_spec_, niq_latency_),
            (long)target_latency_, timestamp_to_ms(input_sample_spec_, target_latency_),
            (double)(fe_ ? fe_->freq_coeff() : 0), (double)freq_coeff_);
}

} // namespace audio
} // namespace roc
