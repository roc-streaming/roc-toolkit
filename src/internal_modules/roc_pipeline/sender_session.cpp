/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_session.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"
#include "roc_fec/codec_map.h"
#include "roc_rtcp/participant_info.h"

namespace roc {
namespace pipeline {

SenderSession::SenderSession(const SenderConfig& config,
                             const rtp::EncodingMap& encoding_map,
                             packet::PacketFactory& packet_factory,
                             core::BufferFactory<uint8_t>& byte_buffer_factory,
                             core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                             core::IArena& arena)
    : arena_(arena)
    , config_(config)
    , encoding_map_(encoding_map)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , frame_writer_(NULL)
    , valid_(false) {
    identity_.reset(new (identity_) rtp::Identity());
    if (!identity_ || !identity_->is_valid()) {
        return;
    }

    valid_ = true;
}

bool SenderSession::is_valid() const {
    return valid_;
}

bool SenderSession::create_transport_pipeline(SenderEndpoint* source_endpoint,
                                              SenderEndpoint* repair_endpoint) {
    roc_panic_if(!is_valid());

    roc_panic_if(!source_endpoint);
    roc_panic_if(frame_writer_);

    const rtp::Encoding* encoding = encoding_map_.find_by_pt(config_.payload_type);
    if (!encoding) {
        return false;
    }

    router_.reset(new (router_) packet::Router(arena_));
    if (!router_) {
        return false;
    }
    packet::IWriter* pwriter = router_.get();

    if (!router_->add_route(source_endpoint->outbound_writer(),
                            packet::Packet::FlagAudio)) {
        return false;
    }

    if (repair_endpoint) {
        if (!router_->add_route(repair_endpoint->outbound_writer(),
                                packet::Packet::FlagRepair)) {
            return false;
        }

        if (config_.enable_interleaving) {
            interleaver_.reset(new (interleaver_) packet::Interleaver(
                *pwriter, arena_,
                config_.fec_writer.n_source_packets
                    + config_.fec_writer.n_repair_packets));
            if (!interleaver_ || !interleaver_->is_valid()) {
                return false;
            }
            pwriter = interleaver_.get();
        }

        fec_encoder_.reset(fec::CodecMap::instance().new_encoder(
                               config_.fec_encoder, byte_buffer_factory_, arena_),
                           arena_);
        if (!fec_encoder_) {
            return false;
        }

        fec_writer_.reset(new (fec_writer_) fec::Writer(
            config_.fec_writer, config_.fec_encoder.scheme, *fec_encoder_, *pwriter,
            source_endpoint->outbound_composer(), repair_endpoint->outbound_composer(),
            packet_factory_, byte_buffer_factory_, arena_));
        if (!fec_writer_ || !fec_writer_->is_valid()) {
            return false;
        }
        pwriter = fec_writer_.get();
    }

    timestamp_extractor_.reset(new (timestamp_extractor_) rtp::TimestampExtractor(
        *pwriter, encoding->sample_spec));
    if (!timestamp_extractor_) {
        return false;
    }
    pwriter = timestamp_extractor_.get();

    payload_encoder_.reset(encoding->new_encoder(arena_, encoding->sample_spec), arena_);
    if (!payload_encoder_) {
        return false;
    }

    sequencer_.reset(new (sequencer_) rtp::Sequencer(*identity_, config_.payload_type));
    if (!sequencer_ || !sequencer_->is_valid()) {
        return false;
    }

    packetizer_.reset(new (packetizer_) audio::Packetizer(
        *pwriter, source_endpoint->outbound_composer(), *sequencer_, *payload_encoder_,
        packet_factory_, byte_buffer_factory_, config_.packet_length,
        encoding->sample_spec));
    if (!packetizer_ || !packetizer_->is_valid()) {
        return false;
    }

    audio::IFrameWriter* awriter = packetizer_.get();

    if (encoding->sample_spec.channel_set() != config_.input_sample_spec.channel_set()) {
        channel_mapper_writer_.reset(
            new (channel_mapper_writer_) audio::ChannelMapperWriter(
                *awriter, sample_buffer_factory_,
                audio::SampleSpec(encoding->sample_spec.sample_rate(),
                                  config_.input_sample_spec.pcm_format(),
                                  config_.input_sample_spec.channel_set()),
                encoding->sample_spec));
        if (!channel_mapper_writer_ || !channel_mapper_writer_->is_valid()) {
            return false;
        }
        awriter = channel_mapper_writer_.get();
    }

    if (config_.latency.tuner_profile != audio::LatencyTunerProfile_Intact
        || encoding->sample_spec.sample_rate()
            != config_.input_sample_spec.sample_rate()) {
        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
            arena_, sample_buffer_factory_, config_.resampler, config_.input_sample_spec,
            audio::SampleSpec(encoding->sample_spec.sample_rate(),
                              config_.input_sample_spec.pcm_format(),
                              config_.input_sample_spec.channel_set())));

        if (!resampler_) {
            return false;
        }

        resampler_writer_.reset(new (resampler_writer_) audio::ResamplerWriter(
            *awriter, *resampler_, sample_buffer_factory_, config_.input_sample_spec,
            audio::SampleSpec(encoding->sample_spec.sample_rate(),
                              config_.input_sample_spec.pcm_format(),
                              config_.input_sample_spec.channel_set())));

        if (!resampler_writer_ || !resampler_writer_->is_valid()) {
            return false;
        }
        awriter = resampler_writer_.get();
    }

    feedback_monitor_.reset(new (feedback_monitor_) audio::FeedbackMonitor(
        *awriter, resampler_writer_.get(), config_.feedback, config_.latency,
        config_.input_sample_spec));
    if (!feedback_monitor_ || !feedback_monitor_->is_valid()) {
        return false;
    }
    awriter = feedback_monitor_.get();

    frame_writer_ = awriter;

    start_feedback_monitor_();

    return true;
}

bool SenderSession::create_control_pipeline(SenderEndpoint* control_endpoint) {
    roc_panic_if(!is_valid());

    roc_panic_if(!control_endpoint);
    roc_panic_if(rtcp_communicator_);

    rtcp_outbound_addr_ = control_endpoint->outbound_address();

    rtcp_communicator_.reset(new (rtcp_communicator_) rtcp::Communicator(
        config_.rtcp, *this, control_endpoint->outbound_writer(),
        control_endpoint->outbound_composer(), packet_factory_, byte_buffer_factory_,
        arena_));
    if (!rtcp_communicator_ || !rtcp_communicator_->is_valid()) {
        rtcp_communicator_.reset();
        return false;
    }

    start_feedback_monitor_();

    return true;
}

audio::IFrameWriter* SenderSession::frame_writer() const {
    roc_panic_if(!is_valid());

    return frame_writer_;
}

status::StatusCode SenderSession::route_packet(const packet::PacketPtr& packet,
                                               core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    if (packet->has_flags(packet::Packet::FlagControl)) {
        return route_control_packet_(packet, current_time);
    }

    roc_panic("sender session: unexpected non-control packet");
}

core::nanoseconds_t SenderSession::refresh(core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    if (rtcp_communicator_) {
        if (has_send_stream()) {
            const status::StatusCode code =
                rtcp_communicator_->generate_reports(current_time);
            // TODO(gh-183): forward status
            roc_panic_if(code != status::StatusOK);
        }

        return rtcp_communicator_->generation_deadline(current_time);
    }

    return 0;
}

SenderSessionMetrics SenderSession::get_metrics() const {
    roc_panic_if(!is_valid());

    SenderSessionMetrics metrics;
    if (packetizer_) {
        metrics.packets = packetizer_->metrics();
    }
    if (feedback_monitor_) {
        metrics.latency = feedback_monitor_->metrics();
    }

    return metrics;
}

rtcp::ParticipantInfo SenderSession::participant_info() {
    rtcp::ParticipantInfo part_info;

    part_info.cname = identity_->cname();
    part_info.source_id = identity_->ssrc();
    part_info.report_mode = rtcp::Report_ToAddress;
    part_info.report_address = rtcp_outbound_addr_;

    return part_info;
}

void SenderSession::change_source_id() {
    identity_->change_ssrc();
}

bool SenderSession::has_send_stream() {
    return timestamp_extractor_ && timestamp_extractor_->has_mapping();
}

rtcp::SendReport SenderSession::query_send_stream(core::nanoseconds_t report_time) {
    roc_panic_if(!has_send_stream());

    const audio::PacketizerMetrics packet_metrics = packetizer_->metrics();

    rtcp::SendReport report;
    report.sender_cname = identity_->cname();
    report.sender_source_id = identity_->ssrc();
    report.report_timestamp = report_time;
    report.stream_timestamp = timestamp_extractor_->get_mapping(report_time);
    report.sample_rate = packetizer_->sample_rate();
    report.packet_count = (uint32_t)packet_metrics.packet_count;
    report.byte_count = (uint32_t)packet_metrics.payload_count;

    return report;
}

status::StatusCode
SenderSession::notify_send_stream(packet::stream_source_t recv_source_id,
                                  const rtcp::RecvReport& recv_report) {
    roc_panic_if(!has_send_stream());

    if (feedback_monitor_ && feedback_monitor_->is_started()) {
        audio::LatencyMetrics metrics;
        metrics.niq_latency = recv_report.niq_latency;
        metrics.niq_stalling = recv_report.niq_stalling;
        metrics.e2e_latency = recv_report.e2e_latency;
        metrics.jitter = recv_report.jitter;

        feedback_monitor_->process_feedback(recv_source_id, metrics);
    }

    return status::StatusOK;
}

void SenderSession::start_feedback_monitor_() {
    if (!feedback_monitor_) {
        // Transport endpoint not created yet.
        return;
    }

    if (!rtcp_communicator_) {
        // Control endpoint not created yet.
        return;
    }

    if (rtcp_outbound_addr_.multicast()) {
        // Control endpoint uses multicast, so there are multiple receivers for
        // a sender session. We don't support feedback monitoring in this mode.
        return;
    }

    if (feedback_monitor_->is_started()) {
        // Already started.
        return;
    }

    feedback_monitor_->start();
}

status::StatusCode
SenderSession::route_control_packet_(const packet::PacketPtr& packet,
                                     core::nanoseconds_t current_time) {
    if (!rtcp_communicator_) {
        roc_panic("sender session: rtcp communicator is null");
    }

    // This will invoke IParticipant methods implemented by us.
    return rtcp_communicator_->process_packet(packet, current_time);
}

} // namespace pipeline
} // namespace roc
