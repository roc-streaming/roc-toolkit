/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_port_group.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_pipeline/port_to_str.h"
#include "roc_pipeline/validate_endpoints.h"

namespace roc {
namespace pipeline {

SenderPortGroup::SenderPortGroup(const SenderConfig& config,
                                 const fec::CodecMap& codec_map,
                                 const rtp::FormatMap& format_map,
                                 packet::PacketPool& packet_pool,
                                 core::BufferPool<uint8_t>& byte_buffer_pool,
                                 core::BufferPool<audio::sample_t>& sample_buffer_pool,
                                 core::IAllocator& allocator)
    : config_(config)
    , codec_map_(codec_map)
    , format_map_(format_map)
    , packet_pool_(packet_pool)
    , byte_buffer_pool_(byte_buffer_pool)
    , sample_buffer_pool_(sample_buffer_pool)
    , allocator_(allocator)
    , audio_writer_(NULL) {
}

void SenderPortGroup::destroy() {
    allocator_.destroy(*this);
}

SenderPort* SenderPortGroup::add_port(address::EndpointType type,
                                      const PortConfig& port_config) {
    SenderPort* created_port = NULL;

    switch ((int)type) {
    case address::EndType_AudioSource:
        if (!(created_port = create_source_port_(port_config))) {
            return NULL;
        }
        break;

    case address::EndType_AudioRepair:
        if (!(created_port = create_repair_port_(port_config))) {
            return NULL;
        }
        break;

    default:
        roc_log(LogError, "sender port group: invalid endpoint protocol");
        return NULL;
    }

    roc_log(LogInfo, "sender port group: created %s endpoint %s",
            address::endpoint_type_to_str(type), port_to_str(port_config).c_str());

    if (source_port_
        && (repair_port_ || config_.fec_encoder.scheme == packet::FEC_None)) {
        if (!create_pipeline_()) {
            return NULL;
        }
    }

    return created_port;
}

audio::IWriter* SenderPortGroup::writer() {
    return audio_writer_;
}

bool SenderPortGroup::is_configured() const {
    return audio_writer_ && source_port_->has_writer()
        && (!repair_port_ || repair_port_->has_writer());
}

SenderPort* SenderPortGroup::create_source_port_(const PortConfig& port_config) {
    if (source_port_) {
        roc_log(LogError, "sender port group: audio source endpoint is already set");
        return NULL;
    }

    if (!validate_transport_endpoint(config_.fec_encoder.scheme,
                                     address::EndType_AudioSource,
                                     port_config.protocol)) {
        return NULL;
    }

    if (repair_port_) {
        if (!validate_transport_endpoint_pair(config_.fec_encoder.scheme,
                                              port_config.protocol,
                                              repair_port_->proto())) {
            return NULL;
        }
    }

    source_port_.reset(new (allocator_) SenderPort(port_config, allocator_), allocator_);
    if (!source_port_ || !source_port_->valid()) {
        roc_log(LogError, "sender port group: can't create source port");
        source_port_.reset();
        return NULL;
    }

    return source_port_.get();
}

SenderPort* SenderPortGroup::create_repair_port_(const PortConfig& port_config) {
    if (repair_port_) {
        roc_log(LogError, "sender port group: audio repair endpoint is already set");
        return NULL;
    }

    if (!validate_transport_endpoint(config_.fec_encoder.scheme,
                                     address::EndType_AudioRepair,
                                     port_config.protocol)) {
        return NULL;
    }

    if (source_port_) {
        if (!validate_transport_endpoint_pair(config_.fec_encoder.scheme,
                                              source_port_->proto(),
                                              port_config.protocol)) {
            return NULL;
        }
    }

    repair_port_.reset(new (allocator_) SenderPort(port_config, allocator_), allocator_);
    if (!repair_port_ || !repair_port_->valid()) {
        roc_log(LogError, "sender port group: can't create repair port");
        repair_port_.reset();
        return NULL;
    }

    return repair_port_.get();
}

bool SenderPortGroup::create_pipeline_() {
    roc_panic_if(audio_writer_);
    roc_panic_if(!source_port_);

    const rtp::Format* format = format_map_.format(config_.payload_type);
    if (!format) {
        return false;
    }

    router_.reset(new (allocator_) packet::Router(allocator_), allocator_);
    if (!router_) {
        return false;
    }
    packet::IWriter* pwriter = router_.get();

    if (!router_->add_route(*source_port_, packet::Packet::FlagAudio)) {
        return false;
    }

    if (repair_port_) {
        if (!router_->add_route(*repair_port_, packet::Packet::FlagRepair)) {
            return false;
        }

        if (config_.interleaving) {
            interleaver_.reset(new (allocator_) packet::Interleaver(
                                   *pwriter, allocator_,
                                   config_.fec_writer.n_source_packets
                                       + config_.fec_writer.n_repair_packets),
                               allocator_);
            if (!interleaver_ || !interleaver_->valid()) {
                return false;
            }
            pwriter = interleaver_.get();
        }

        fec_encoder_.reset(
            codec_map_.new_encoder(config_.fec_encoder, byte_buffer_pool_, allocator_),
            allocator_);
        if (!fec_encoder_) {
            return false;
        }

        fec_writer_.reset(
            new (allocator_)
                fec::Writer(config_.fec_writer, config_.fec_encoder.scheme, *fec_encoder_,
                            *pwriter, source_port_->composer(), repair_port_->composer(),
                            packet_pool_, byte_buffer_pool_, allocator_),
            allocator_);
        if (!fec_writer_ || !fec_writer_->valid()) {
            return false;
        }
        pwriter = fec_writer_.get();
    }

    payload_encoder_.reset(format->new_encoder(allocator_), allocator_);
    if (!payload_encoder_) {
        return false;
    }

    packetizer_.reset(new (allocator_) audio::Packetizer(
                          *pwriter, source_port_->composer(), *payload_encoder_,
                          packet_pool_, byte_buffer_pool_, config_.input_channels,
                          config_.packet_length, format->sample_rate,
                          config_.payload_type),
                      allocator_);
    if (!packetizer_) {
        return false;
    }

    audio::IWriter* awriter = packetizer_.get();

    if (config_.resampling && config_.input_sample_rate != format->sample_rate) {
        if (config_.poisoning) {
            resampler_poisoner_.reset(new (allocator_) audio::PoisonWriter(*awriter),
                                      allocator_);
            if (!resampler_poisoner_) {
                return false;
            }
            awriter = resampler_poisoner_.get();
        }

        audio::ResamplerMap resampler_map;

        resampler_.reset(resampler_map.new_resampler(
                             config_.resampler_backend, allocator_, config_.resampler,
                             config_.input_channels, config_.internal_frame_size),
                         allocator_);

        if (!resampler_) {
            return false;
        }

        resampler_writer_.reset(
            new (allocator_) audio::ResamplerWriter(
                *awriter, *resampler_, sample_buffer_pool_, config_.internal_frame_size),
            allocator_);

        if (!resampler_writer_ || !resampler_writer_->valid()) {
            return false;
        }
        if (!resampler_writer_->set_scaling(float(config_.input_sample_rate)
                                            / format->sample_rate)) {
            return false;
        }
        awriter = resampler_writer_.get();
    }

    audio_writer_ = awriter;

    return true;
}

} // namespace pipeline
} // namespace roc
