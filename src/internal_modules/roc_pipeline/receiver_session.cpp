/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_session.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/codec_map.h"

namespace roc {
namespace pipeline {

ReceiverSession::ReceiverSession(const ReceiverSessionConfig& session_config,
                                 const ReceiverCommonConfig& common_config,
                                 const rtp::EncodingMap& encoding_map,
                                 packet::PacketFactory& packet_factory,
                                 audio::FrameFactory& frame_factory,
                                 core::IArena& arena)
    : core::RefCounted<ReceiverSession, core::ArenaAllocation>(arena)
    , frame_reader_(NULL)
    , valid_(false) {
    const rtp::Encoding* pkt_encoding =
        encoding_map.find_by_pt(session_config.payload_type);
    if (!pkt_encoding) {
        return;
    }

    packet_router_.reset(new (packet_router_) packet::Router(arena));
    if (!packet_router_) {
        return;
    }

    // First part of pipeline: chained packet writers from endpoint to queues.
    // Endpoint writes packets to this pipeline, and in the end it stores
    // packets in the queues.
    packet::IWriter* pkt_writer = NULL;

    source_queue_.reset(new (source_queue_) packet::SortedQueue(0));
    if (!source_queue_) {
        return;
    }
    pkt_writer = source_queue_.get();

    source_meter_.reset(new (source_meter_) rtp::LinkMeter(encoding_map));
    if (!source_meter_) {
        return;
    }
    source_meter_->set_writer(*pkt_writer);
    pkt_writer = source_meter_.get();

    if (!packet_router_->add_route(*pkt_writer, packet::Packet::FlagAudio)) {
        return;
    }

    // Second part of pipeline: chained packet readers from queues to depacketizer.
    // Depacketizer reads packets from this pipeline, and in the end it reads
    // packets stored in the queues.
    packet::IReader* pkt_reader = source_queue_.get();

    payload_decoder_.reset(pkt_encoding->new_decoder(arena, pkt_encoding->sample_spec),
                           arena);
    if (!payload_decoder_) {
        return;
    }

    filter_.reset(new (filter_)
                      rtp::Filter(*pkt_reader, *payload_decoder_,
                                  common_config.rtp_filter, pkt_encoding->sample_spec));
    if (!filter_) {
        return;
    }
    pkt_reader = filter_.get();

    delayed_reader_.reset(new (delayed_reader_) packet::DelayedReader(
        *pkt_reader, pkt_encoding->sample_spec));

    if (!delayed_reader_ || !delayed_reader_->is_valid()) {
        return;
    }
    pkt_reader = delayed_reader_.get();

    source_meter_->set_reader(*pkt_reader);
    pkt_reader = source_meter_.get();

    if (session_config.fec_decoder.scheme != packet::FEC_None) {
        repair_queue_.reset(new (repair_queue_) packet::SortedQueue(0));
        if (!repair_queue_) {
            return;
        }

        repair_meter_.reset(new (repair_meter_) rtp::LinkMeter(encoding_map));
        if (!repair_meter_) {
            return;
        }
        repair_meter_->set_writer(*repair_queue_);

        if (!packet_router_->add_route(*repair_meter_, packet::Packet::FlagRepair)) {
            return;
        }

        fec_decoder_.reset(fec::CodecMap::instance().new_decoder(
                               session_config.fec_decoder, packet_factory, arena),
                           arena);
        if (!fec_decoder_) {
            return;
        }

        fec_parser_.reset(new (fec_parser_) rtp::Parser(encoding_map, NULL));
        if (!fec_parser_) {
            return;
        }

        fec_reader_.reset(new (fec_reader_) fec::Reader(
            session_config.fec_reader, session_config.fec_decoder.scheme, *fec_decoder_,
            *pkt_reader, *repair_queue_, *fec_parser_, packet_factory, arena));
        if (!fec_reader_ || !fec_reader_->is_valid()) {
            return;
        }
        pkt_reader = fec_reader_.get();

        fec_filter_.reset(new (fec_filter_) rtp::Filter(*pkt_reader, *payload_decoder_,
                                                        common_config.rtp_filter,
                                                        pkt_encoding->sample_spec));
        if (!fec_filter_) {
            return;
        }
        pkt_reader = fec_filter_.get();

        repair_meter_->set_reader(*pkt_reader);
        pkt_reader = repair_meter_.get();
    }

    timestamp_injector_.reset(new (timestamp_injector_) rtp::TimestampInjector(
        *pkt_reader, pkt_encoding->sample_spec));
    if (!timestamp_injector_) {
        return;
    }
    pkt_reader = timestamp_injector_.get();

    // Third part of pipeline: chained frame readers from depacketizer to mixer.
    // Mixed reads frames from this pipeline, and in the end it requests packets
    // from packet readers pipeline.
    audio::IFrameReader* frm_reader = NULL;

    {
        const audio::SampleSpec out_spec(pkt_encoding->sample_spec.sample_rate(),
                                         audio::Sample_RawFormat,
                                         pkt_encoding->sample_spec.channel_set());

        depacketizer_.reset(new (depacketizer_) audio::Depacketizer(
            *pkt_reader, *payload_decoder_, out_spec, session_config.enable_beeping));
        if (!depacketizer_ || !depacketizer_->is_valid()) {
            return;
        }
        frm_reader = depacketizer_.get();

        if (session_config.watchdog.no_playback_timeout >= 0
            || session_config.watchdog.choppy_playback_timeout >= 0) {
            watchdog_.reset(new (watchdog_) audio::Watchdog(
                *frm_reader, out_spec, session_config.watchdog, arena));
            if (!watchdog_ || !watchdog_->is_valid()) {
                return;
            }
            frm_reader = watchdog_.get();
        }
    }

    if (pkt_encoding->sample_spec.channel_set()
        != common_config.output_sample_spec.channel_set()) {
        const audio::SampleSpec in_spec(pkt_encoding->sample_spec.sample_rate(),
                                        audio::Sample_RawFormat,
                                        pkt_encoding->sample_spec.channel_set());

        const audio::SampleSpec out_spec(pkt_encoding->sample_spec.sample_rate(),
                                         audio::Sample_RawFormat,
                                         common_config.output_sample_spec.channel_set());

        channel_mapper_reader_.reset(
            new (channel_mapper_reader_) audio::ChannelMapperReader(
                *frm_reader, frame_factory, in_spec, out_spec));
        if (!channel_mapper_reader_ || !channel_mapper_reader_->is_valid()) {
            return;
        }
        frm_reader = channel_mapper_reader_.get();
    }

    if (session_config.latency.tuner_profile != audio::LatencyTunerProfile_Intact
        || pkt_encoding->sample_spec.sample_rate()
            != common_config.output_sample_spec.sample_rate()) {
        const audio::SampleSpec in_spec(pkt_encoding->sample_spec.sample_rate(),
                                        audio::Sample_RawFormat,
                                        common_config.output_sample_spec.channel_set());

        const audio::SampleSpec out_spec(common_config.output_sample_spec.sample_rate(),
                                         audio::Sample_RawFormat,
                                         common_config.output_sample_spec.channel_set());

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
            arena, frame_factory, session_config.resampler, in_spec, out_spec));
        if (!resampler_) {
            return;
        }

        resampler_reader_.reset(new (resampler_reader_) audio::ResamplerReader(
            *frm_reader, *resampler_, in_spec, out_spec));
        if (!resampler_reader_ || !resampler_reader_->is_valid()) {
            return;
        }
        frm_reader = resampler_reader_.get();
    }

    latency_monitor_.reset(new (latency_monitor_) audio::LatencyMonitor(
        *frm_reader, *source_queue_, *depacketizer_, *source_meter_,
        resampler_reader_.get(), session_config.latency, pkt_encoding->sample_spec,
        common_config.output_sample_spec, *delayed_reader_.get()));
    if (!latency_monitor_ || !latency_monitor_->is_valid()) {
        return;
    }
    frm_reader = latency_monitor_.get();

    if (!frm_reader) {
        return;
    }

    // Top-level frame reader that is added to mixer.
    frame_reader_ = frm_reader;
    valid_ = true;
}

bool ReceiverSession::is_valid() const {
    return valid_;
}

audio::IFrameReader& ReceiverSession::frame_reader() {
    roc_panic_if(!is_valid());

    return *frame_reader_;
}

status::StatusCode ReceiverSession::route_packet(const packet::PacketPtr& packet) {
    roc_panic_if(!is_valid());

    return packet_router_->write(packet);
}

bool ReceiverSession::refresh(core::nanoseconds_t current_time,
                              core::nanoseconds_t* next_refresh) {
    roc_panic_if(!is_valid());

    (void)current_time;

    if (next_refresh) {
        *next_refresh = 0;
    }

    if (watchdog_) {
        if (!watchdog_->is_alive()) {
            return false;
        }
    }

    if (!latency_monitor_->is_alive()) {
        return false;
    }

    return true;
}

bool ReceiverSession::reclock(core::nanoseconds_t playback_time) {
    roc_panic_if(!is_valid());

    return latency_monitor_->reclock(playback_time);
}

size_t ReceiverSession::num_reports() const {
    roc_panic_if(!is_valid());

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
    roc_panic_if(!is_valid());

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
        report.packet_count = link_metrics.total_packets;
        report.cum_loss = link_metrics.lost_packets;
        report.jitter = link_metrics.jitter;
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
        report.packet_count = link_metrics.total_packets;
        report.cum_loss = link_metrics.lost_packets;
        report.jitter = link_metrics.jitter;

        reports++;
        n_reports--;
    }
}

void ReceiverSession::process_report(const rtcp::SendReport& report) {
    roc_panic_if(!is_valid());

    if (packet_router_->has_source_id(packet::Packet::FlagAudio)
        && packet_router_->get_source_id(packet::Packet::FlagAudio)
            == report.sender_source_id) {
        source_meter_->process_report(report);

        timestamp_injector_->update_mapping(report.report_timestamp,
                                            report.stream_timestamp);
    }
}

ReceiverParticipantMetrics ReceiverSession::get_metrics() const {
    roc_panic_if(!is_valid());

    ReceiverParticipantMetrics metrics;
    metrics.link = source_meter_->metrics();
    metrics.latency = latency_monitor_->metrics();

    return metrics;
}

} // namespace pipeline
} // namespace roc
