/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_endpoint_set.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/codec_map.h"
#include "roc_pipeline/endpoint_helpers.h"

namespace roc {
namespace pipeline {

SenderEndpointSet::SenderEndpointSet(
    const SenderConfig& config,
    const rtp::FormatMap& format_map,
    packet::PacketPool& packet_pool,
    core::BufferPool<uint8_t>& byte_buffer_pool,
    core::BufferPool<audio::sample_t>& sample_buffer_pool,
    core::IAllocator& allocator)
    : config_(config)
    , format_map_(format_map)
    , packet_pool_(packet_pool)
    , byte_buffer_pool_(byte_buffer_pool)
    , sample_buffer_pool_(sample_buffer_pool)
    , allocator_(allocator)
    , audio_writer_(NULL) {
}

void SenderEndpointSet::destroy() {
    allocator_.destroy(*this);
}

SenderEndpoint* SenderEndpointSet::add_endpoint(address::Interface iface,
                                                address::Protocol proto) {
    roc_log(LogDebug, "sender endpoint set: adding %s endpoint %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    SenderEndpoint* endpoint = NULL;

    switch ((int)iface) {
    case address::Iface_AudioSource:
        if (!(endpoint = create_source_endpoint_(proto))) {
            return NULL;
        }
        break;

    case address::Iface_AudioRepair:
        if (!(endpoint = create_repair_endpoint_(proto))) {
            return NULL;
        }
        break;

    default:
        roc_log(LogError, "sender endpoint set: unsupported interface");
        return NULL;
    }

    if (source_endpoint_
        && (repair_endpoint_ || config_.fec_encoder.scheme == packet::FEC_None)) {
        if (!create_pipeline_()) {
            return NULL;
        }
    }

    return endpoint;
}

audio::IWriter* SenderEndpointSet::writer() {
    return audio_writer_;
}

bool SenderEndpointSet::is_ready() const {
    return audio_writer_ && source_endpoint_->has_writer()
        && (!repair_endpoint_ || repair_endpoint_->has_writer());
}

SenderEndpoint* SenderEndpointSet::create_source_endpoint_(address::Protocol proto) {
    if (source_endpoint_) {
        roc_log(LogError, "sender endpoint set: audio source endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioSource, proto)) {
        return NULL;
    }

    if (repair_endpoint_) {
        if (!validate_endpoint_pair_consistency(proto, repair_endpoint_->proto())) {
            return NULL;
        }
    }

    if (!validate_endpoint_and_pipeline_consistency(config_.fec_encoder.scheme,
                                                    address::Iface_AudioSource, proto)) {
        return NULL;
    }

    source_endpoint_.reset(new (allocator_) SenderEndpoint(proto, allocator_),
                           allocator_);
    if (!source_endpoint_ || !source_endpoint_->valid()) {
        roc_log(LogError, "sender endpoint set: can't create source endpoint");
        source_endpoint_.reset();
        return NULL;
    }

    return source_endpoint_.get();
}

SenderEndpoint* SenderEndpointSet::create_repair_endpoint_(address::Protocol proto) {
    if (repair_endpoint_) {
        roc_log(LogError, "sender endpoint set: audio repair endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioRepair, proto)) {
        return NULL;
    }

    if (source_endpoint_) {
        if (!validate_endpoint_pair_consistency(source_endpoint_->proto(), proto)) {
            return NULL;
        }
    }

    if (!validate_endpoint_and_pipeline_consistency(config_.fec_encoder.scheme,
                                                    address::Iface_AudioRepair, proto)) {
        return NULL;
    }

    repair_endpoint_.reset(new (allocator_) SenderEndpoint(proto, allocator_),
                           allocator_);
    if (!repair_endpoint_ || !repair_endpoint_->valid()) {
        roc_log(LogError, "sender endpoint set: can't create repair endpoint");
        repair_endpoint_.reset();
        return NULL;
    }

    return repair_endpoint_.get();
}

bool SenderEndpointSet::create_pipeline_() {
    roc_panic_if(audio_writer_);
    roc_panic_if(!source_endpoint_);

    const rtp::Format* format = format_map_.format(config_.payload_type);
    if (!format) {
        return false;
    }

    router_.reset(new (allocator_) packet::Router(allocator_), allocator_);
    if (!router_) {
        return false;
    }
    packet::IWriter* pwriter = router_.get();

    if (!router_->add_route(*source_endpoint_, packet::Packet::FlagAudio)) {
        return false;
    }

    if (repair_endpoint_) {
        if (!router_->add_route(*repair_endpoint_, packet::Packet::FlagRepair)) {
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

        fec_encoder_.reset(fec::CodecMap::instance().new_encoder(
                               config_.fec_encoder, byte_buffer_pool_, allocator_),
                           allocator_);
        if (!fec_encoder_) {
            return false;
        }

        fec_writer_.reset(new (allocator_) fec::Writer(
                              config_.fec_writer, config_.fec_encoder.scheme,
                              *fec_encoder_, *pwriter, source_endpoint_->composer(),
                              repair_endpoint_->composer(), packet_pool_,
                              byte_buffer_pool_, allocator_),
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
                          *pwriter, source_endpoint_->composer(), *payload_encoder_,
                          packet_pool_, byte_buffer_pool_, config_.input_channels,
                          config_.packet_length, format->sample_rate,
                          config_.payload_type),
                      allocator_);
    if (!packetizer_ || !packetizer_->valid()) {
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

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
                             config_.resampler_backend, allocator_, config_.resampler,
                             config_.internal_frame_length, config_.input_sample_rate,
                             config_.input_channels),
                         allocator_);

        if (!resampler_) {
            return false;
        }

        resampler_writer_.reset(new (allocator_) audio::ResamplerWriter(
                                    *awriter, *resampler_, sample_buffer_pool_,
                                    config_.internal_frame_length,
                                    config_.input_sample_rate, config_.input_channels),
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
