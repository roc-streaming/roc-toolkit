/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_session.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/codec_map.h"

namespace roc {
namespace pipeline {

SenderSession::SenderSession(const SenderSinkConfig& sink_config,
                             audio::ProcessorMap& processor_map,
                             rtp::EncodingMap& encoding_map,
                             packet::PacketFactory& packet_factory,
                             audio::FrameFactory& frame_factory,
                             core::IArena& arena,
                             dbgio::CsvDumper* dumper)
    : arena_(arena)
    , sink_config_(sink_config)
    , processor_map_(processor_map)
    , encoding_map_(encoding_map)
    , packet_factory_(packet_factory)
    , frame_factory_(frame_factory)
    , frame_writer_(NULL)
    , dumper_(dumper)
    , init_status_(status::NoStatus)
    , fail_status_(status::NoStatus) {
    identity_.reset(new (identity_) rtp::Identity());
    if ((init_status_ = identity_->init_status()) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode SenderSession::init_status() const {
    return init_status_;
}

status::StatusCode
SenderSession::create_transport_pipeline(SenderEndpoint* source_endpoint,
                                         SenderEndpoint* repair_endpoint) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!source_endpoint);
    roc_panic_if(frame_writer_);

    status::StatusCode status = status::NoStatus;

    const rtp::Encoding* pkt_encoding =
        encoding_map_.find_by_pt(sink_config_.payload_type);
    if (!pkt_encoding) {
        roc_log(LogError,
                "sender session: can't find registered encoding for payload id %u",
                (unsigned)sink_config_.payload_type);
        return status::StatusBadConfig;
    }

    // First part of pipeline: chained packet writers from packetizer to endpoint.
    // Packetizer writes packet to this pipeline, and it the end it writes
    // packets into endpoint outbound writers.
    packet::IWriter* pkt_writer = NULL;

    router_.reset(new (router_) packet::Router(arena_));
    if ((status = router_->init_status()) != status::StatusOK) {
        return status;
    }
    pkt_writer = router_.get();

    if ((status = router_->add_route(source_endpoint->outbound_writer(),
                                     packet::Packet::FlagAudio))
        != status::StatusOK) {
        return status;
    }

    if (repair_endpoint) {
        if ((status = router_->add_route(repair_endpoint->outbound_writer(),
                                         packet::Packet::FlagRepair))
            != status::StatusOK) {
            return status;
        }

        if (sink_config_.enable_interleaving) {
            interleaver_.reset(new (interleaver_) packet::Interleaver(
                *pkt_writer, arena_,
                sink_config_.fec_writer.n_source_packets
                    + sink_config_.fec_writer.n_repair_packets));
            if ((status = interleaver_->init_status()) != status::StatusOK) {
                return status;
            }
            pkt_writer = interleaver_.get();
        }

        fec_encoder_.reset(fec::CodecMap::instance().new_block_encoder(
            sink_config_.fec_encoder, packet_factory_, arena_));
        if (!fec_encoder_) {
            return status::StatusNoMem;
        }
        if ((status = fec_encoder_->init_status()) != status::StatusOK) {
            return status;
        }

        fec_writer_.reset(new (fec_writer_) fec::BlockWriter(
            sink_config_.fec_writer, sink_config_.fec_encoder.scheme, *fec_encoder_,
            *pkt_writer, source_endpoint->outbound_composer(),
            repair_endpoint->outbound_composer(), packet_factory_, arena_));
        if ((status = fec_writer_->init_status()) != status::StatusOK) {
            return status;
        }
        pkt_writer = fec_writer_.get();
    }

    timestamp_extractor_.reset(new (timestamp_extractor_) rtp::TimestampExtractor(
        *pkt_writer, pkt_encoding->sample_spec));
    if ((status = timestamp_extractor_->init_status()) != status::StatusOK) {
        return status;
    }
    pkt_writer = timestamp_extractor_.get();

    payload_encoder_.reset(pkt_encoding->new_encoder(pkt_encoding->sample_spec, arena_));
    if (!payload_encoder_) {
        return status::StatusNoMem;
    }
    if ((status = payload_encoder_->init_status()) != status::StatusOK) {
        return status;
    }

    sequencer_.reset(new (sequencer_)
                         rtp::Sequencer(*identity_, sink_config_.payload_type));
    if ((status = sequencer_->init_status()) != status::StatusOK) {
        return status;
    }

    // Second part of pipeline: chained frame writers from fanout to packetizer.
    // Fanout writes frames to this pipeline, and in the end it writes packets
    // to packet writers pipeline.
    audio::IFrameWriter* frm_writer = NULL;

    {
        const audio::SampleSpec in_spec(pkt_encoding->sample_spec.sample_rate(),
                                        audio::PcmSubformat_Raw,
                                        pkt_encoding->sample_spec.channel_set());

        packetizer_.reset(new (packetizer_) audio::Packetizer(
            *pkt_writer, source_endpoint->outbound_composer(), *sequencer_,
            *payload_encoder_, packet_factory_, sink_config_.packet_length, in_spec));
        if ((status = packetizer_->init_status()) != status::StatusOK) {
            return status;
        }
        frm_writer = packetizer_.get();
    }

    if (pkt_encoding->sample_spec.channel_set()
        != sink_config_.input_sample_spec.channel_set()) {
        const audio::SampleSpec in_spec(pkt_encoding->sample_spec.sample_rate(),
                                        audio::PcmSubformat_Raw,
                                        sink_config_.input_sample_spec.channel_set());

        const audio::SampleSpec out_spec(pkt_encoding->sample_spec.sample_rate(),
                                         audio::PcmSubformat_Raw,
                                         pkt_encoding->sample_spec.channel_set());

        channel_mapper_writer_.reset(
            new (channel_mapper_writer_) audio::ChannelMapperWriter(
                *frm_writer, frame_factory_, in_spec, out_spec));
        if ((status = channel_mapper_writer_->init_status()) != status::StatusOK) {
            return status;
        }
        frm_writer = channel_mapper_writer_.get();
    }

    if (sink_config_.latency.tuner_profile != audio::LatencyTunerProfile_Intact
        || pkt_encoding->sample_spec.sample_rate()
            != sink_config_.input_sample_spec.sample_rate()) {
        const audio::SampleSpec in_spec(sink_config_.input_sample_spec.sample_rate(),
                                        audio::PcmSubformat_Raw,
                                        sink_config_.input_sample_spec.channel_set());

        const audio::SampleSpec out_spec(pkt_encoding->sample_spec.sample_rate(),
                                         audio::PcmSubformat_Raw,
                                         sink_config_.input_sample_spec.channel_set());

        resampler_.reset(processor_map_.new_resampler(sink_config_.resampler, in_spec,
                                                      out_spec, frame_factory_, arena_));
        if (!resampler_) {
            return status::StatusNoMem;
        }
        if ((status = resampler_->init_status()) != status::StatusOK) {
            return status;
        }

        resampler_writer_.reset(new (resampler_writer_) audio::ResamplerWriter(
            *frm_writer, frame_factory_, *resampler_, in_spec, out_spec));

        if ((status = resampler_writer_->init_status()) != status::StatusOK) {
            return status;
        }
        frm_writer = resampler_writer_.get();
    }

    {
        const audio::SampleSpec inout_spec(sink_config_.input_sample_spec.sample_rate(),
                                           audio::PcmSubformat_Raw,
                                           sink_config_.input_sample_spec.channel_set());

        feedback_monitor_.reset(new (feedback_monitor_) audio::FeedbackMonitor(
            *frm_writer, *packetizer_, resampler_writer_.get(), sink_config_.feedback,
            sink_config_.latency, sink_config_.freq_est, inout_spec, dumper_));
        if ((status = feedback_monitor_->init_status()) != status::StatusOK) {
            return status;
        }
        frm_writer = feedback_monitor_.get();
    }

    // Top-level frame writer that is added to fanout.
    frame_writer_ = frm_writer;
    start_feedback_monitor_();
    return status::StatusOK;
}

status::StatusCode
SenderSession::create_control_pipeline(SenderEndpoint* control_endpoint) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!control_endpoint);
    roc_panic_if(rtcp_communicator_);

    rtcp_outbound_addr_ = control_endpoint->outbound_address();

    rtcp_communicator_.reset(new (rtcp_communicator_) rtcp::Communicator(
        sink_config_.rtcp, *this, control_endpoint->outbound_writer(),
        control_endpoint->outbound_composer(), packet_factory_, arena_));

    const status::StatusCode code = rtcp_communicator_->init_status();
    if (code != status::StatusOK) {
        rtcp_communicator_.reset();
        return code;
    }

    start_feedback_monitor_();

    return status::StatusOK;
}

audio::IFrameWriter* SenderSession::frame_writer() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!frame_writer_) {
        return NULL;
    }

    return this;
}

status::StatusCode SenderSession::refresh(core::nanoseconds_t current_time,
                                          core::nanoseconds_t& next_deadline) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (fail_status_ != status::NoStatus) {
        // Report remembered error code.
        return fail_status_;
    }

    if (rtcp_communicator_) {
        if (has_send_stream()) {
            const status::StatusCode code =
                rtcp_communicator_->generate_reports(current_time);
            if (code != status::StatusOK) {
                return code;
            }
        }
        next_deadline = rtcp_communicator_->generation_deadline(current_time);
    }

    return status::StatusOK;
}

status::StatusCode SenderSession::route_packet(const packet::PacketPtr& packet,
                                               core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (fail_status_ != status::NoStatus) {
        // Session broken.
        return status::StatusNoRoute;
    }

    if (!packet->has_flags(packet::Packet::FlagControl)) {
        roc_panic("sender session: unexpected non-control packet");
    }

    return route_control_packet_(packet, current_time);
}

status::StatusCode SenderSession::write(audio::Frame& frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (fail_status_ != status::NoStatus) {
        // Session broken.
        return status::StatusFinish;
    }

    const status::StatusCode code = frame_writer_->write(frame);

    // On failure, mark session broken and return StatusFinish to be excluded from fanout.
    // Error will be reported later from refresh().
    if (code != status::StatusOK) {
        // These codes can't be returned from write().
        roc_panic_if_msg(code == status::StatusPart || code == status::StatusDrain,
                         "sender session: unexpected status code %s",
                         status::code_to_str(code));
        fail_status_ = code;
        return status::StatusFinish;
    }

    return code;
}

void SenderSession::get_slot_metrics(SenderSlotMetrics& slot_metrics) const {
    roc_panic_if(init_status_ != status::StatusOK);

    slot_metrics.source_id = identity_->ssrc();
    slot_metrics.num_participants =
        feedback_monitor_ ? feedback_monitor_->num_participants() : 0;
    slot_metrics.is_complete = (frame_writer_ != NULL);
}

void SenderSession::get_participant_metrics(SenderParticipantMetrics* party_metrics,
                                            size_t* party_count) const {
    roc_panic_if(init_status_ != status::StatusOK);

    if (party_metrics && party_count) {
        *party_count = std::min(
            *party_count, feedback_monitor_ ? feedback_monitor_->num_participants() : 0);

        for (size_t n_part = 0; n_part < *party_count; n_part++) {
            party_metrics[n_part].link = feedback_monitor_->link_metrics(n_part);
            party_metrics[n_part].latency = feedback_monitor_->latency_metrics(n_part);
        }
    } else if (party_count) {
        *party_count = 0;
    }
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
    const status::StatusCode code = identity_->change_ssrc();

    if (code != status::StatusOK) {
        roc_panic("sender session: can't change SSRC: status=%s",
                  status::code_to_str(code));
    }
}

bool SenderSession::has_send_stream() {
    return timestamp_extractor_ && timestamp_extractor_->has_mapping();
}

rtcp::SendReport SenderSession::query_send_stream(core::nanoseconds_t report_time) {
    roc_panic_if(!has_send_stream());

    const audio::PacketizerMetrics& packet_metrics = packetizer_->metrics();

    rtcp::SendReport report;
    report.sender_cname = identity_->cname();
    report.sender_source_id = identity_->ssrc();
    report.report_timestamp = report_time;
    report.stream_timestamp = timestamp_extractor_->get_mapping(report_time);
    report.sample_rate = packetizer_->sample_rate();
    report.packet_count = packet_metrics.encoded_packets;
    report.byte_count = packet_metrics.payload_bytes;

    return report;
}

status::StatusCode
SenderSession::notify_send_stream(packet::stream_source_t recv_source_id,
                                  const rtcp::RecvReport& recv_report) {
    roc_panic_if(!has_send_stream());

    if (feedback_monitor_ && feedback_monitor_->is_started()) {
        audio::LatencyMetrics latency_metrics;
        latency_metrics.niq_latency = recv_report.niq_latency;
        latency_metrics.niq_stalling = recv_report.niq_stalling;
        latency_metrics.e2e_latency = recv_report.e2e_latency;

        packet::LinkMetrics link_metrics;
        link_metrics.ext_first_seqnum = recv_report.ext_first_seqnum;
        link_metrics.ext_last_seqnum = recv_report.ext_last_seqnum;
        link_metrics.expected_packets = recv_report.packet_count;
        link_metrics.lost_packets = recv_report.cum_loss;
        link_metrics.peak_jitter = recv_report.jitter;
        link_metrics.rtt = recv_report.rtt;

        feedback_monitor_->process_feedback(recv_source_id, latency_metrics,
                                            link_metrics);
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
