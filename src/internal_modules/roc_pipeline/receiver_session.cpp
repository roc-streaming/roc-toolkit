/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_session.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/codec_map.h"

namespace roc {
namespace pipeline {

ReceiverSession::ReceiverSession(const ReceiverSessionConfig& session_config,
                                 const ReceiverCommonConfig& common_config,
                                 audio::ProcessorMap& processor_map,
                                 rtp::EncodingMap& encoding_map,
                                 packet::PacketFactory& packet_factory,
                                 audio::FrameFactory& frame_factory,
                                 core::IArena& arena,
                                 dbgio::CsvDumper* dumper)
    : core::RefCounted<ReceiverSession, core::ArenaAllocation>(arena)
    , frame_reader_(NULL)
    , dumper_(dumper)
    , init_status_(status::NoStatus)
    , fail_status_(status::NoStatus) {
    const rtp::Encoding* pkt_encoding =
        encoding_map.find_by_pt(session_config.payload_type);
    if (!pkt_encoding) {
        roc_log(LogError,
                "receiver session: can't find registered encoding for payload id %u",
                (unsigned)session_config.payload_type);
        init_status_ = status::StatusNoRoute;
        return;
    }

    packet_router_.reset(new (packet_router_) packet::Router(arena));
    if ((init_status_ = packet_router_->init_status()) != status::StatusOK) {
        return;
    }

    // First part of pipeline: chained packet writers from endpoint to queues.
    // Endpoint writes packets to this pipeline, and in the end it stores
    // packets in the queues.
    packet::IWriter* pkt_writer = NULL;

    source_queue_.reset(new (source_queue_) packet::SortedQueue(0));
    if ((init_status_ = source_queue_->init_status()) != status::StatusOK) {
        return;
    }
    pkt_writer = source_queue_.get();

    source_meter_.reset(new (source_meter_) rtp::LinkMeter(
        *pkt_writer, session_config.jitter_meter, encoding_map, arena, dumper_));
    if ((init_status_ = source_meter_->init_status()) != status::StatusOK) {
        return;
    }
    pkt_writer = source_meter_.get();

    if ((init_status_ = packet_router_->add_route(*pkt_writer, packet::Packet::FlagAudio))
        != status::StatusOK) {
        return;
    }

    // Second part of pipeline: chained packet readers from queues to depacketizer.
    // Depacketizer reads packets from this pipeline, and in the end it reads
    // packets stored in the queues.
    packet::IReader* pkt_reader = source_queue_.get();

    payload_decoder_.reset(pkt_encoding->new_decoder(pkt_encoding->sample_spec, arena));
    if (!payload_decoder_) {
        init_status_ = status::StatusNoMem;
        return;
    }
    if ((init_status_ = payload_decoder_->init_status()) != status::StatusOK) {
        return;
    }

    filter_.reset(new (filter_)
                      rtp::Filter(*pkt_reader, *payload_decoder_,
                                  common_config.rtp_filter, pkt_encoding->sample_spec));
    if ((init_status_ = filter_->init_status()) != status::StatusOK) {
        return;
    }
    pkt_reader = filter_.get();

    delayed_reader_.reset(new (delayed_reader_) packet::DelayedReader(
        *pkt_reader,
        session_config.latency.target_latency != 0
            ? session_config.latency.target_latency
            : session_config.latency.start_target_latency,
        pkt_encoding->sample_spec));
    if ((init_status_ = delayed_reader_->init_status()) != status::StatusOK) {
        return;
    }
    pkt_reader = delayed_reader_.get();

    if (session_config.fec_decoder.scheme != packet::FEC_None) {
        // Sub-pipeline with chained writers for repair packets.
        packet::IWriter* repair_pkt_writer = NULL;

        repair_queue_.reset(new (repair_queue_) packet::SortedQueue(0));
        if ((init_status_ = repair_queue_->init_status()) != status::StatusOK) {
            return;
        }
        repair_pkt_writer = repair_queue_.get();

        repair_meter_.reset(new (repair_meter_) rtp::LinkMeter(
            *repair_pkt_writer, session_config.jitter_meter, encoding_map, arena,
            dumper_));
        if ((init_status_ = repair_meter_->init_status()) != status::StatusOK) {
            return;
        }
        repair_pkt_writer = repair_meter_.get();

        if ((init_status_ = packet_router_->add_route(*repair_pkt_writer,
                                                      packet::Packet::FlagRepair))
            != status::StatusOK) {
            return;
        }

        // Sub-pipeline with chained readers for packets after repairing losses.
        fec_decoder_.reset(fec::CodecMap::instance().new_block_decoder(
            session_config.fec_decoder, packet_factory, arena));
        if (!fec_decoder_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_decoder_->init_status()) != status::StatusOK) {
            return;
        }

        fec_parser_.reset(new (fec_parser_) rtp::Parser(NULL, encoding_map, arena));
        if ((init_status_ = fec_parser_->init_status()) != status::StatusOK) {
            return;
        }

        fec_reader_.reset(new (fec_reader_) fec::BlockReader(
            session_config.fec_reader, session_config.fec_decoder.scheme, *fec_decoder_,
            *pkt_reader, *repair_queue_, *fec_parser_, packet_factory, arena));
        if ((init_status_ = fec_reader_->init_status()) != status::StatusOK) {
            return;
        }
        pkt_reader = fec_reader_.get();

        fec_filter_.reset(new (fec_filter_) rtp::Filter(*pkt_reader, *payload_decoder_,
                                                        common_config.rtp_filter,
                                                        pkt_encoding->sample_spec));
        if ((init_status_ = fec_filter_->init_status()) != status::StatusOK) {
            return;
        }
        pkt_reader = fec_filter_.get();
    }

    timestamp_injector_.reset(new (timestamp_injector_) rtp::TimestampInjector(
        *pkt_reader, pkt_encoding->sample_spec));
    if ((init_status_ = timestamp_injector_->init_status()) != status::StatusOK) {
        return;
    }
    pkt_reader = timestamp_injector_.get();

    // Third part of pipeline: chained frame readers from depacketizer to mixer.
    // Mixed reads frames from this pipeline, and in the end it requests packets
    // from packet readers pipeline.
    audio::IFrameReader* frm_reader = NULL;

    {
        const audio::SampleSpec out_spec(pkt_encoding->sample_spec.sample_rate(),
                                         audio::PcmSubformat_Raw,
                                         pkt_encoding->sample_spec.channel_set());

        depacketizer_.reset(new (depacketizer_) audio::Depacketizer(
            *pkt_reader, *payload_decoder_, frame_factory, out_spec, dumper_));
        if ((init_status_ = depacketizer_->init_status()) != status::StatusOK) {
            return;
        }
        frm_reader = depacketizer_.get();

        if (session_config.plc.backend != audio::PlcBackend_None) {
            plc_.reset(processor_map.new_plc(session_config.plc, out_spec, frame_factory,
                                             arena));
            if (!plc_) {
                init_status_ = status::StatusNoMem;
                return;
            }
            if ((init_status_ = plc_->init_status()) != status::StatusOK) {
                return;
            }

            plc_reader_.reset(new (plc_reader_) audio::PlcReader(
                *frm_reader, frame_factory, *plc_, out_spec));
            if ((init_status_ = plc_reader_->init_status()) != status::StatusOK) {
                return;
            }
            frm_reader = plc_reader_.get();
        }

        if (session_config.watchdog.no_playback_timeout >= 0
            || session_config.watchdog.choppy_playback_timeout >= 0) {
            watchdog_.reset(new (watchdog_) audio::Watchdog(
                *frm_reader, out_spec, session_config.watchdog, arena));
            if ((init_status_ = watchdog_->init_status()) != status::StatusOK) {
                return;
            }
            frm_reader = watchdog_.get();
        }
    }

    if (pkt_encoding->sample_spec.channel_set()
        != common_config.output_sample_spec.channel_set()) {
        const audio::SampleSpec in_spec(pkt_encoding->sample_spec.sample_rate(),
                                        audio::PcmSubformat_Raw,
                                        pkt_encoding->sample_spec.channel_set());

        const audio::SampleSpec out_spec(pkt_encoding->sample_spec.sample_rate(),
                                         audio::PcmSubformat_Raw,
                                         common_config.output_sample_spec.channel_set());

        channel_mapper_reader_.reset(
            new (channel_mapper_reader_) audio::ChannelMapperReader(
                *frm_reader, frame_factory, in_spec, out_spec));
        if ((init_status_ = channel_mapper_reader_->init_status()) != status::StatusOK) {
            return;
        }
        frm_reader = channel_mapper_reader_.get();
    }

    if (session_config.latency.tuner_profile != audio::LatencyTunerProfile_Intact
        || pkt_encoding->sample_spec.sample_rate()
            != common_config.output_sample_spec.sample_rate()) {
        const audio::SampleSpec in_spec(pkt_encoding->sample_spec.sample_rate(),
                                        audio::PcmSubformat_Raw,
                                        common_config.output_sample_spec.channel_set());

        const audio::SampleSpec out_spec(common_config.output_sample_spec.sample_rate(),
                                         audio::PcmSubformat_Raw,
                                         common_config.output_sample_spec.channel_set());

        resampler_.reset(processor_map.new_resampler(session_config.resampler, in_spec,
                                                     out_spec, frame_factory, arena));
        if (!resampler_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = resampler_->init_status()) != status::StatusOK) {
            return;
        }

        resampler_reader_.reset(new (resampler_reader_) audio::ResamplerReader(
            *frm_reader, frame_factory, *resampler_, in_spec, out_spec));
        if ((init_status_ = resampler_reader_->init_status()) != status::StatusOK) {
            return;
        }
        frm_reader = resampler_reader_.get();
    }

    {
        const audio::SampleSpec inout_spec(
            common_config.output_sample_spec.sample_rate(), audio::PcmSubformat_Raw,
            common_config.output_sample_spec.channel_set());

        latency_monitor_.reset(new (latency_monitor_) audio::LatencyMonitor(
            *frm_reader, *source_queue_, *depacketizer_, *source_meter_,
            fec_reader_.get(), resampler_reader_.get(), session_config.latency,
            session_config.freq_est, pkt_encoding->sample_spec, inout_spec, dumper_));
        if ((init_status_ = latency_monitor_->init_status()) != status::StatusOK) {
            return;
        }
        frm_reader = latency_monitor_.get();
    }

    // Top-level frame reader that is added to mixer.
    frame_reader_ = frm_reader;
    init_status_ = status::StatusOK;
}

status::StatusCode ReceiverSession::init_status() const {
    return init_status_;
}

audio::IFrameReader& ReceiverSession::frame_reader() {
    roc_panic_if(init_status_ != status::StatusOK);

    return *this;
}

status::StatusCode ReceiverSession::refresh(core::nanoseconds_t current_time,
                                            core::nanoseconds_t& next_deadline) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (fail_status_ != status::NoStatus) {
        // Report remembered error code.
        return fail_status_;
    }

    return status::StatusOK;
}

void ReceiverSession::reclock(core::nanoseconds_t playback_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (fail_status_ != status::NoStatus) {
        // Session broken.
        return;
    }

    latency_monitor_->reclock(playback_time);
}

status::StatusCode ReceiverSession::route_packet(const packet::PacketPtr& packet) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (fail_status_ != status::NoStatus) {
        // Session broken.
        return status::StatusNoRoute;
    }

    return packet_router_->write(packet);
}

status::StatusCode ReceiverSession::read(audio::Frame& frame,
                                         packet::stream_timestamp_t duration,
                                         audio::FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (fail_status_ != status::NoStatus) {
        // Session broken.
        return status::StatusFinish;
    }

    const status::StatusCode code = frame_reader_->read(frame, duration, mode);

    // On failure, mark session broken and return StatusFinish to be excluded from mixer.
    // Error will be reported later from refresh().
    if (code != status::StatusOK && code != status::StatusPart
        && code != status::StatusDrain) {
        fail_status_ = code;
        return status::StatusFinish;
    }

    return code;
}

size_t ReceiverSession::num_reports() const {
    roc_panic_if(init_status_ != status::StatusOK);

    size_t n_reports = 0;

    if (packet_router_->has_source_id(packet::Packet::FlagAudio)
        && source_meter_->has_metrics()) {
        n_reports++;
    }

    if (packet_router_->has_source_id(packet::Packet::FlagRepair)
        && repair_meter_->has_metrics()) {
        n_reports++;
    }

    return n_reports;
}

void ReceiverSession::generate_reports(const char* report_cname,
                                       packet::stream_source_t report_ssrc,
                                       core::nanoseconds_t report_time,
                                       rtcp::RecvReport* reports,
                                       size_t n_reports) const {
    roc_panic_if(init_status_ != status::StatusOK);

    if (n_reports > 0 && packet_router_->has_source_id(packet::Packet::FlagAudio)
        && source_meter_->has_metrics() && source_meter_->has_encoding()) {
        const audio::LatencyMetrics& latency_metrics = latency_monitor_->metrics();
        const packet::LinkMetrics& link_metrics = source_meter_->metrics();

        rtcp::RecvReport& report = *reports;

        report.receiver_cname = report_cname;
        report.receiver_source_id = report_ssrc;
        report.sender_source_id =
            packet_router_->get_source_id(packet::Packet::FlagAudio);
        report.report_timestamp = report_time;
        report.sample_rate = source_meter_->encoding().sample_spec.sample_rate();
        report.ext_first_seqnum = link_metrics.ext_first_seqnum;
        report.ext_last_seqnum = link_metrics.ext_last_seqnum;
        report.packet_count = link_metrics.expected_packets;
        report.cum_loss = link_metrics.lost_packets;
        report.jitter = link_metrics.peak_jitter;
        report.niq_latency = latency_metrics.niq_latency;
        report.niq_stalling = latency_metrics.niq_stalling;
        report.e2e_latency = latency_metrics.e2e_latency;

        reports++;
        n_reports--;
    }

    if (n_reports > 0 && packet_router_->has_source_id(packet::Packet::FlagRepair)
        && repair_meter_->has_metrics() && repair_meter_->has_encoding()) {
        const packet::LinkMetrics& link_metrics = repair_meter_->metrics();

        rtcp::RecvReport& report = *reports;

        report.receiver_cname = report_cname;
        report.receiver_source_id = report_ssrc;
        report.sender_source_id =
            packet_router_->get_source_id(packet::Packet::FlagRepair);
        report.report_timestamp = report_time;
        report.sample_rate = repair_meter_->encoding().sample_spec.sample_rate();
        report.ext_first_seqnum = link_metrics.ext_first_seqnum;
        report.ext_last_seqnum = link_metrics.ext_last_seqnum;
        report.packet_count = link_metrics.expected_packets;
        report.cum_loss = link_metrics.lost_packets;
        report.jitter = link_metrics.peak_jitter;

        reports++;
        n_reports--;
    }
}

void ReceiverSession::process_report(const rtcp::SendReport& report) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (packet_router_->has_source_id(packet::Packet::FlagAudio)
        && packet_router_->get_source_id(packet::Packet::FlagAudio)
            == report.sender_source_id) {
        source_meter_->process_report(report);

        timestamp_injector_->update_mapping(report.report_timestamp,
                                            report.stream_timestamp);
    }
}

ReceiverParticipantMetrics ReceiverSession::get_metrics() const {
    roc_panic_if(init_status_ != status::StatusOK);

    ReceiverParticipantMetrics metrics;
    metrics.link = source_meter_->metrics();
    metrics.latency = latency_monitor_->metrics();
    metrics.depacketizer = depacketizer_->metrics();

    return metrics;
}

} // namespace pipeline
} // namespace roc
