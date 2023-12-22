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
#include "roc_core/time.h"
#include "roc_fec/codec_map.h"

namespace roc {
namespace pipeline {

ReceiverSession::ReceiverSession(
    const ReceiverSessionConfig& session_config,
    const ReceiverCommonConfig& common_config,
    const address::SocketAddr& src_address,
    const rtp::EncodingMap& encoding_map,
    packet::PacketFactory& packet_factory,
    core::BufferFactory<uint8_t>& byte_buffer_factory,
    core::BufferFactory<audio::sample_t>& sample_buffer_factory,
    core::IArena& arena)
    : core::RefCounted<ReceiverSession, core::ArenaAllocation>(arena)
    , src_address_(src_address)
    , audio_reader_(NULL) {
    const rtp::Encoding* encoding = encoding_map.find_by_pt(session_config.payload_type);
    if (!encoding) {
        return;
    }

    queue_router_.reset(new (queue_router_) packet::Router(arena));
    if (!queue_router_) {
        return;
    }

    source_queue_.reset(new (source_queue_) packet::SortedQueue(0));
    if (!source_queue_) {
        return;
    }

    packet::IWriter* pwriter = source_queue_.get();

    if (!queue_router_->add_route(*pwriter, packet::Packet::FlagAudio)) {
        return;
    }

    packet::IReader* preader = source_queue_.get();

    payload_decoder_.reset(
        encoding->new_decoder(arena, encoding->pcm_format, encoding->sample_spec), arena);
    if (!payload_decoder_) {
        return;
    }

    validator_.reset(new (validator_) rtp::Validator(
        *preader, session_config.rtp_validator, encoding->sample_spec));
    if (!validator_) {
        return;
    }
    preader = validator_.get();

    populator_.reset(new (populator_) rtp::Populator(*preader, *payload_decoder_,
                                                     encoding->sample_spec));
    if (!populator_) {
        return;
    }
    preader = populator_.get();

    delayed_reader_.reset(new (delayed_reader_) packet::DelayedReader(
        *preader, session_config.target_latency, encoding->sample_spec));
    if (!delayed_reader_) {
        return;
    }
    preader = delayed_reader_.get();

    if (session_config.fec_decoder.scheme != packet::FEC_None) {
        repair_queue_.reset(new (repair_queue_) packet::SortedQueue(0));
        if (!repair_queue_) {
            return;
        }
        if (!queue_router_->add_route(*repair_queue_, packet::Packet::FlagRepair)) {
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

        fec_validator_.reset(new (fec_validator_) rtp::Validator(
            *preader, session_config.rtp_validator, encoding->sample_spec));
        if (!fec_validator_) {
            return;
        }
        preader = fec_validator_.get();

        fec_populator_.reset(new (fec_populator_) rtp::Populator(
            *preader, *payload_decoder_, encoding->sample_spec));
        if (!fec_populator_) {
            return;
        }
        preader = fec_populator_.get();
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

    audio_reader_ = areader;
}

bool ReceiverSession::is_valid() const {
    return audio_reader_;
}

status::StatusCode ReceiverSession::route(const packet::PacketPtr& packet) {
    roc_panic_if(!is_valid());

    packet::UDP* udp = packet->udp();
    if (!udp) {
        // TODO(gh-183): return StatusNoRoute
        return status::StatusUnknown;
    }

    if (udp->src_addr != src_address_) {
        // TODO(gh-183): return StatusNoRoute
        return status::StatusUnknown;
    }

    return queue_router_->write(packet);
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

ReceiverSessionMetrics ReceiverSession::get_metrics() const {
    roc_panic_if(!is_valid());

    ReceiverSessionMetrics metrics;
    metrics.latency = latency_monitor_->metrics();

    return metrics;
}

audio::IFrameReader& ReceiverSession::reader() {
    roc_panic_if(!is_valid());

    return *audio_reader_;
}

void ReceiverSession::add_sending_metrics(const rtcp::SendingMetrics& metrics) {
    roc_panic_if(!is_valid());

    timestamp_injector_->update_mapping(metrics.origin_time, metrics.origin_rtp);
}

void ReceiverSession::add_link_metrics(const rtcp::LinkMetrics& metrics) {
    // TODO
    (void)metrics;
}

} // namespace pipeline
} // namespace roc
