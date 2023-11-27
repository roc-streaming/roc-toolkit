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
#include "roc_core/time.h"
#include "roc_fec/codec_map.h"
#include "roc_rtcp/communicator.h"

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
    , audio_writer_(NULL)
    , num_sources_(0) {
}

bool SenderSession::create_transport_pipeline(SenderEndpoint* source_endpoint,
                                              SenderEndpoint* repair_endpoint) {
    roc_panic_if(audio_writer_);
    roc_panic_if(!source_endpoint);

    if (source_endpoint) {
        num_sources_++;
    }

    if (repair_endpoint) {
        num_sources_++;
    }

    const rtp::Encoding* encoding = encoding_map_.find_by_pt(config_.payload_type);
    if (!encoding) {
        return false;
    }

    router_.reset(new (router_) packet::Router(arena_));
    if (!router_) {
        return false;
    }
    packet::IWriter* pwriter = router_.get();

    if (!router_->add_route(source_endpoint->writer(), packet::Packet::FlagAudio)) {
        return false;
    }

    if (repair_endpoint) {
        if (!router_->add_route(repair_endpoint->writer(), packet::Packet::FlagRepair)) {
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
            source_endpoint->composer(), repair_endpoint->composer(), packet_factory_,
            byte_buffer_factory_, arena_));
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

    payload_encoder_.reset(
        encoding->new_encoder(arena_, encoding->pcm_format, encoding->sample_spec),
        arena_);
    if (!payload_encoder_) {
        return false;
    }

    packetizer_.reset(new (packetizer_) audio::Packetizer(
        *pwriter, source_endpoint->composer(), *payload_encoder_, packet_factory_,
        byte_buffer_factory_, config_.packet_length, encoding->sample_spec,
        config_.payload_type));
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

    if (encoding->sample_spec.sample_rate() != config_.input_sample_spec.sample_rate()) {
        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
            config_.resampler_backend, arena_, sample_buffer_factory_,
            config_.resampler_profile, config_.input_sample_spec,
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

    audio_writer_ = awriter;

    return true;
}

bool SenderSession::create_control_pipeline(SenderEndpoint* control_endpoint) {
    roc_panic_if(rtcp_communicator_);
    roc_panic_if(!control_endpoint);

    rtcp_composer_.reset(new (rtcp_composer_) rtcp::Composer());
    if (!rtcp_composer_) {
        return false;
    }

    rtcp_communicator_.reset(new (rtcp_communicator_) rtcp::Communicator(
        config_.rtcp_config, *this, &control_endpoint->writer(), *rtcp_composer_,
        packet_factory_, byte_buffer_factory_, arena_));
    if (!rtcp_communicator_ || !rtcp_communicator_->is_valid()) {
        return false;
    }

    return true;
}

audio::IFrameWriter* SenderSession::writer() const {
    return audio_writer_;
}

core::nanoseconds_t SenderSession::refresh(core::nanoseconds_t current_time) {
    if (!rtcp_communicator_) {
        return 0;
    }

    if (timestamp_extractor_ && timestamp_extractor_->has_mapping()) {
        const status::StatusCode code =
            rtcp_communicator_->generate_reports(current_time);

        // TODO(gh-183): forward status
        roc_panic_if(code != status::StatusOK);
    }

    return rtcp_communicator_->generation_deadline(current_time);
}

SenderSessionMetrics SenderSession::get_metrics() const {
    SenderSessionMetrics metrics;
    return metrics;
}

const char* SenderSession::cname() {
    // TODO
    return "todo";
}

packet::stream_source_t SenderSession::source_id() {
    // TODO
    return 123;
}

void SenderSession::change_source_id() {
    // TODO
}

bool SenderSession::has_send_stream() {
    return true;
}

rtcp::SendReport SenderSession::query_send_stream(core::nanoseconds_t report_time) {
    // TODO
    rtcp::SendReport report;
    report.sender_cname = cname();
    report.sender_source_id = source_id();
    report.report_timestamp = report_time;
    report.stream_timestamp = timestamp_extractor_->get_mapping(report_time);
    return report;
}

void SenderSession::notify_send_stream(packet::stream_source_t recv_source_id,
                                       const rtcp::RecvReport& recv_report) {
    // TODO
}

} // namespace pipeline
} // namespace roc
