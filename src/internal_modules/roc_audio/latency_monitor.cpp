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
#include "roc_core/time.h"

namespace roc {
namespace audio {

LatencyMonitor::LatencyMonitor(IFrameReader& frame_reader,
                               const packet::SortedQueue& incoming_queue,
                               const Depacketizer& depacketizer,
                               const packet::ILinkMeter& link_meter,
                               const fec::BlockReader* fec_reader,
                               ResamplerReader* resampler,
                               const LatencyConfig& latency_config,
                               const FreqEstimatorConfig& fe_config,
                               const SampleSpec& packet_sample_spec,
                               const SampleSpec& frame_sample_spec,
                               dbgio::CsvDumper* dumper)
    : tuner_(latency_config, fe_config, frame_sample_spec, dumper)
    , frame_reader_(frame_reader)
    , incoming_queue_(incoming_queue)
    , depacketizer_(depacketizer)
    , link_meter_(link_meter)
    , fec_reader_(fec_reader)
    , resampler_(resampler)
    , enable_scaling_(latency_config.tuner_profile != audio::LatencyTunerProfile_Intact)
    , capture_ts_(0)
    , packet_sample_spec_(packet_sample_spec)
    , frame_sample_spec_(frame_sample_spec)
    , init_status_(status::NoStatus) {
    if ((init_status_ = tuner_.init_status()) != status::StatusOK) {
        return;
    }

    if (enable_scaling_) {
        if (!init_scaling_()) {
            init_status_ = status::StatusBadConfig;
            return;
        }
    }

    init_status_ = status::StatusOK;
}

status::StatusCode LatencyMonitor::init_status() const {
    return init_status_;
}

const LatencyMetrics& LatencyMonitor::metrics() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return latency_metrics_;
}

status::StatusCode LatencyMonitor::read(Frame& frame,
                                        packet::stream_timestamp_t duration,
                                        FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    compute_niq_latency_();
    query_metrics_();

    if (!pre_read_()) {
        return status::StatusAbort;
    }

    const status::StatusCode code = frame_reader_.read(frame, duration, mode);
    if (code != status::StatusOK && code != status::StatusPart) {
        return code;
    }

    frame_sample_spec_.validate_frame(frame);

    post_read_(frame);

    return status::StatusOK;
}

void LatencyMonitor::reclock(const core::nanoseconds_t playback_timestamp) {
    roc_panic_if(init_status_ != status::StatusOK);

    // this method is called when playback time of last frame was reported
    // now we can update e2e latency based on it
    compute_e2e_latency_(playback_timestamp);
}

bool LatencyMonitor::pre_read_() {
    tuner_.write_metrics(latency_metrics_, link_metrics_);

    if (!tuner_.update_stream()) {
        return false;
    }

    if (enable_scaling_) {
        if (!update_scaling_()) {
            return false;
        }
    }

    return true;
}

void LatencyMonitor::post_read_(const Frame& frame) {
    // for end-2-end latency calculations
    capture_ts_ = frame.capture_timestamp();

    // after reading the frame we know its duration
    tuner_.advance_stream(frame.duration());
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
    const packet::stream_timestamp_t niq_tail =
        latest_packet->stream_timestamp() + latest_packet->duration();

    // packet pipeline length
    // includes incoming queue and packets buffered inside other packet
    // pipeline elements, e.g. in FEC reader
    const packet::stream_timestamp_diff_t niq_latency =
        packet::stream_timestamp_diff(niq_tail, niq_head);

    latency_metrics_.niq_latency =
        packet_sample_spec_.stream_timestamp_delta_2_ns(niq_latency);

    // compute delay since last packet
    const core::nanoseconds_t rts = latest_packet->receive_timestamp();
    const core::nanoseconds_t now = core::timestamp(core::ClockUnix);

    if (rts > 0 && rts < now) {
        latency_metrics_.niq_stalling = now - rts;
    }
}

void LatencyMonitor::compute_e2e_latency_(const core::nanoseconds_t playback_timestamp) {
    if (capture_ts_ == 0) {
        return;
    }

    if (playback_timestamp <= 0) {
        return;
    }

    // delta between time when first sample of last frame is played on receiver and
    // time when first sample of that frame was captured on sender
    // (both timestamps are in receiver clock domain)
    latency_metrics_.e2e_latency = playback_timestamp - capture_ts_;
}

void LatencyMonitor::query_metrics_() {
    if (link_meter_.has_metrics()) {
        link_metrics_ = link_meter_.metrics();
    }

    if (fec_reader_) {
        latency_metrics_.fec_block_duration =
            packet_sample_spec_.stream_timestamp_2_ns(fec_reader_->max_block_duration());
    }
}

bool LatencyMonitor::init_scaling_() {
    roc_panic_if_not(resampler_);

    if (!resampler_->set_scaling(1.0f)) {
        roc_log(LogError, "latency monitor: can't set initial scaling");
        return false;
    }

    return true;
}

bool LatencyMonitor::update_scaling_() {
    roc_panic_if_not(resampler_);

    const float scaling = tuner_.fetch_scaling();
    if (scaling > 0) {
        if (!resampler_->set_scaling(scaling)) {
            roc_log(LogDebug,
                    "latency monitor: scaling factor out of bounds: scaling=%.6f",
                    (double)scaling);
            return false;
        }
    }

    return true;
}

} // namespace audio
} // namespace roc
