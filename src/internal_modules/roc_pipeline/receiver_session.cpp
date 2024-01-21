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

ReceiverSession::ReceiverSession(
    const ReceiverSessionConfig& session_config,
    const ReceiverCommonConfig& common_config,
    const rtp::EncodingMap& encoding_map,
    packet::PacketFactory& packet_factory,
    core::BufferFactory<uint8_t>& byte_buffer_factory,
    core::BufferFactory<audio::sample_t>& sample_buffer_factory,
    core::IArena& arena)
    : core::RefCounted<ReceiverSession, core::ArenaAllocation>(arena)
    , frame_reader_(NULL)
    , valid_(false) {
    const rtp::Encoding* encoding = encoding_map.find_by_pt(session_config.payload_type);
    if (!encoding) {
        return;
    }

    packet_router_.reset(new (packet_router_) packet::Router(arena));
    if (!packet_router_) {
        return;
    }

    source_queue_.reset(new (source_queue_) packet::SortedQueue(0));
    if (!source_queue_) {
        return;
    }

    packet::IWriter* pwriter = source_queue_.get();

    source_meter_.reset(new (source_meter_) rtp::LinkMeter());
    if (!source_meter_) {
        return;
    }
    source_meter_->set_writer(*pwriter);
    pwriter = source_meter_.get();

    if (!packet_router_->add_route(*pwriter, packet::Packet::FlagAudio)) {
        return;
    }

    packet::IReader* preader = source_queue_.get();

    payload_decoder_.reset(
        encoding->new_decoder(arena, encoding->pcm_format, encoding->sample_spec), arena);
    if (!payload_decoder_) {
        return;
    }

    filter_.reset(new (filter_) rtp::Filter(
        *preader, *payload_decoder_, session_config.rtp_filter, encoding->sample_spec));
    if (!filter_) {
        return;
    }
    preader = filter_.get();

    delayed_reader_.reset(new (delayed_reader_) packet::DelayedReader(
        *preader, session_config.target_latency, encoding->sample_spec));
    if (!delayed_reader_) {
        return;
    }
    preader = delayed_reader_.get();

    source_meter_->set_reader(*preader);
    preader = source_meter_.get();

    if (session_config.fec_decoder.scheme != packet::FEC_None) {
        repair_queue_.reset(new (repair_queue_) packet::SortedQueue(0));
        if (!repair_queue_) {
            return;
        }

        repair_meter_.reset(new (repair_meter_) rtp::LinkMeter());
        if (!repair_meter_) {
            return;
        }
        repair_meter_->set_writer(*repair_queue_);

        if (!packet_router_->add_route(*repair_meter_, packet::Packet::FlagRepair)) {
            return;
        }

        fec_decoder_.reset(fec::CodecMap::instance().new_decoder(
                               session_config.fec_decoder, byte_buffer_factory, arena),
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
            *preader, *repair_queue_, *fec_parser_, packet_factory, arena));
        if (!fec_reader_ || !fec_reader_->is_valid()) {
            return;
        }
        preader = fec_reader_.get();

        fec_filter_.reset(new (fec_filter_) rtp::Filter(*preader, *payload_decoder_,
                                                        session_config.rtp_filter,
                                                        encoding->sample_spec));
        if (!fec_filter_) {
            return;
        }
        preader = fec_filter_.get();

        repair_meter_->set_reader(*preader);
        preader = repair_meter_.get();
    }

    timestamp_injector_.reset(new (timestamp_injector_) rtp::TimestampInjector(
        *preader, encoding->sample_spec));
    if (!timestamp_injector_) {
        return;
    }
    preader = timestamp_injector_.get();

    depacketizer_.reset(new (depacketizer_) audio::Depacketizer(
        *preader, *payload_decoder_, encoding->sample_spec,
        common_config.enable_beeping));
    if (!depacketizer_ || !depacketizer_->is_valid()) {
        return;
    }

    audio::IFrameReader* areader = depacketizer_.get();

    if (session_config.watchdog.no_playback_timeout != 0
        || session_config.watchdog.choppy_playback_timeout != 0
        || session_config.watchdog.frame_status_window != 0) {
        watchdog_.reset(new (watchdog_) audio::Watchdog(*areader, encoding->sample_spec,
                                                        session_config.watchdog, arena));
        if (!watchdog_ || !watchdog_->is_valid()) {
            return;
        }
        areader = watchdog_.get();
    }

    if (encoding->sample_spec.channel_set()
        != common_config.output_sample_spec.channel_set()) {
        channel_mapper_reader_.reset(
            new (channel_mapper_reader_) audio::ChannelMapperReader(
                *areader, sample_buffer_factory, encoding->sample_spec,
                audio::SampleSpec(encoding->sample_spec.sample_rate(),
                                  common_config.output_sample_spec.pcm_format(),
                                  common_config.output_sample_spec.channel_set())));
        if (!channel_mapper_reader_ || !channel_mapper_reader_->is_valid()) {
            return;
        }
        areader = channel_mapper_reader_.get();
    }

    if (session_config.latency_monitor.fe_enable
        || encoding->sample_spec.sample_rate()
            != common_config.output_sample_spec.sample_rate()) {
        resampler_poisoner_.reset(new (resampler_poisoner_)
                                      audio::PoisonReader(*areader));
        if (!resampler_poisoner_) {
            return;
        }
        areader = resampler_poisoner_.get();

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
            session_config.resampler_backend, arena, sample_buffer_factory,
            session_config.resampler_profile,
            audio::SampleSpec(encoding->sample_spec.sample_rate(),
                              common_config.output_sample_spec.pcm_format(),
                              common_config.output_sample_spec.channel_set()),
            common_config.output_sample_spec));

        if (!resampler_) {
            return;
        }

        resampler_reader_.reset(new (resampler_reader_) audio::ResamplerReader(
            *areader, *resampler_,
            audio::SampleSpec(encoding->sample_spec.sample_rate(),
                              common_config.output_sample_spec.pcm_format(),
                              common_config.output_sample_spec.channel_set()),
            common_config.output_sample_spec));

        if (!resampler_reader_ || !resampler_reader_->is_valid()) {
            return;
        }
        areader = resampler_reader_.get();
    }

    session_poisoner_.reset(new (session_poisoner_) audio::PoisonReader(*areader));
    if (!session_poisoner_) {
        return;
    }
    areader = session_poisoner_.get();

    latency_monitor_.reset(new (latency_monitor_) audio::LatencyMonitor(
        *areader, *source_queue_, *depacketizer_, resampler_reader_.get(),
        session_config.latency_monitor, session_config.target_latency,
        encoding->sample_spec, common_config.output_sample_spec));
    if (!latency_monitor_ || !latency_monitor_->is_valid()) {
        return;
    }
    areader = latency_monitor_.get();

    if (!areader) {
        return;
    }

    frame_reader_ = areader;
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
        && source_meter_->has_metrics()) {
        const audio::LatencyMonitorMetrics latency_metrics = latency_monitor_->metrics();
        const rtp::LinkMetrics link_metrics = source_meter_->metrics();

        rtcp::RecvReport& report = *reports;

        report.receiver_cname = report_cname;
        report.receiver_source_id = report_ssrc;
        report.sender_source_id =
            packet_router_->get_source_id(packet::Packet::FlagAudio);
        report.report_timestamp = report_time;
        report.ext_first_seqnum = link_metrics.ext_first_seqnum;
        report.ext_last_seqnum = link_metrics.ext_last_seqnum;
        report.fract_loss = link_metrics.fract_loss;
        report.cum_loss = link_metrics.cum_loss;
        report.jitter = link_metrics.jitter;
        report.e2e_latency = latency_metrics.e2e_latency;

        reports++;
        n_reports--;
    }

    if (n_reports > 0 && packet_router_->has_source_id(packet::Packet::FlagRepair)
        && repair_meter_->has_metrics()) {
        const rtp::LinkMetrics link_metrics = repair_meter_->metrics();

        rtcp::RecvReport& report = *reports;

        report.receiver_cname = report_cname;
        report.receiver_source_id = report_ssrc;
        report.sender_source_id =
            packet_router_->get_source_id(packet::Packet::FlagRepair);
        report.report_timestamp = report_time;
        report.ext_first_seqnum = link_metrics.ext_first_seqnum;
        report.ext_last_seqnum = link_metrics.ext_last_seqnum;
        report.fract_loss = link_metrics.fract_loss;
        report.cum_loss = link_metrics.cum_loss;
        report.jitter = link_metrics.jitter;
        report.e2e_latency = 0;

        reports++;
        n_reports--;
    }
}

void ReceiverSession::process_report(const rtcp::SendReport& report) {
    roc_panic_if(!is_valid());

    if (packet_router_->has_source_id(packet::Packet::FlagAudio)
        && packet_router_->get_source_id(packet::Packet::FlagAudio)
            == report.sender_source_id) {
        timestamp_injector_->update_mapping(report.report_timestamp,
                                            report.stream_timestamp);
    }
}

ReceiverSessionMetrics ReceiverSession::get_metrics() const {
    roc_panic_if(!is_valid());

    ReceiverSessionMetrics metrics;
    metrics.link = source_meter_->metrics();
    metrics.latency = latency_monitor_->metrics();

    return metrics;
}

} // namespace pipeline
} // namespace roc
