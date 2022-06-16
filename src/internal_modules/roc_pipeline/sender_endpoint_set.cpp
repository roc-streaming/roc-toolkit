/*
 * Copyright (c) 2020 Roc Streaming authors
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
    audio::Fanout& fanout,
    packet::PacketFactory& packet_factory,
    core::BufferFactory<uint8_t>& byte_buffer_factory,
    core::BufferFactory<audio::sample_t>& sample_buffer_factory,
    core::IAllocator& allocator)
    : RefCounted(allocator)
    , config_(config)
    , format_map_(format_map)
    , fanout_(fanout)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , audio_writer_(NULL) {
}

SenderEndpoint* SenderEndpointSet::create_endpoint(address::Interface iface,
                                                   address::Protocol proto) {
    roc_log(LogDebug, "sender endpoint set: adding %s endpoint %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    SenderEndpoint* endpoint = NULL;

    switch (iface) {
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

    case address::Iface_AudioControl:
        if (!(endpoint = create_control_endpoint_(proto))) {
            return NULL;
        }
        break;

    default:
        roc_log(LogError, "sender endpoint set: unsupported interface");
        return NULL;
    }

    switch (iface) {
    case address::Iface_AudioSource:
    case address::Iface_AudioRepair:
        if (source_endpoint_
            && (repair_endpoint_ || config_.fec_encoder.scheme == packet::FEC_None)) {
            if (!create_transport_pipeline_()) {
                return NULL;
            }
        }
        if (audio_writer_) {
            if (!fanout_.has_output(*audio_writer_)) {
                fanout_.add_output(*audio_writer_);
            }
        }
        break;

    case address::Iface_AudioControl:
        if (control_endpoint_) {
            if (!create_control_pipeline_()) {
                return NULL;
            }
        }
        break;

    default:
        break;
    }

    return endpoint;
}

audio::IWriter* SenderEndpointSet::writer() {
    return audio_writer_;
}

bool SenderEndpointSet::is_ready() const {
    return audio_writer_ && source_endpoint_->has_destination_writer()
        && (!repair_endpoint_ || repair_endpoint_->has_destination_writer());
}

core::nanoseconds_t SenderEndpointSet::get_update_deadline() const {
    if (rtcp_session_) {
        return rtcp_session_->generation_deadline();
    }

    return 0;
}

void SenderEndpointSet::update() {
    if (rtcp_session_) {
        rtcp_session_->generate_packets();
    }
}

size_t SenderEndpointSet::num_sending_sources() {
    return !!source_endpoint_ + !!repair_endpoint_;
}

packet::source_t SenderEndpointSet::get_sending_source(size_t source_index) {
    switch (source_index) {
    case 0:
        // TODO
        return 123;

    case 1:
        // TODO
        return 456;
    }

    roc_panic("sender endpoint set: source index out of bounds: source_index=%lu",
              (unsigned long)source_index);
}

rtcp::SendingMetrics
SenderEndpointSet::get_sending_metrics(packet::ntp_timestamp_t report_time) {
    // TODO

    rtcp::SendingMetrics metrics;
    metrics.origin_ntp = report_time;

    return metrics;
}

void SenderEndpointSet::add_reception_metrics(const rtcp::ReceptionMetrics& metrics) {
    // TODO

    (void)metrics;
}

void SenderEndpointSet::add_link_metrics(const rtcp::LinkMetrics& metrics) {
    // TODO

    (void)metrics;
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

    source_endpoint_.reset(new (source_endpoint_) SenderEndpoint(proto, allocator()));
    if (!source_endpoint_ || !source_endpoint_->valid()) {
        roc_log(LogError, "sender endpoint set: can't create source endpoint");
        source_endpoint_.reset(NULL);
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

    repair_endpoint_.reset(new (repair_endpoint_) SenderEndpoint(proto, allocator()));
    if (!repair_endpoint_ || !repair_endpoint_->valid()) {
        roc_log(LogError, "sender endpoint set: can't create repair endpoint");
        repair_endpoint_.reset(NULL);
        return NULL;
    }

    return repair_endpoint_.get();
}

SenderEndpoint* SenderEndpointSet::create_control_endpoint_(address::Protocol proto) {
    if (control_endpoint_) {
        roc_log(LogError, "sender endpoint set: audio control endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioControl, proto)) {
        return NULL;
    }

    control_endpoint_.reset(new (control_endpoint_) SenderEndpoint(proto, allocator()));
    if (!control_endpoint_ || !control_endpoint_->valid()) {
        roc_log(LogError, "sender endpoint set: can't create control endpoint");
        control_endpoint_.reset(NULL);
        return NULL;
    }

    return control_endpoint_.get();
}

bool SenderEndpointSet::create_transport_pipeline_() {
    roc_panic_if(audio_writer_);
    roc_panic_if(!source_endpoint_);

    const rtp::Format* format = format_map_.format(config_.payload_type);
    if (!format) {
        return false;
    }

    router_.reset(new (router_) packet::Router(allocator()));
    if (!router_) {
        return false;
    }
    packet::IWriter* pwriter = router_.get();

    if (!router_->add_route(source_endpoint_->writer(), packet::Packet::FlagAudio)) {
        return false;
    }

    if (repair_endpoint_) {
        if (!router_->add_route(repair_endpoint_->writer(), packet::Packet::FlagRepair)) {
            return false;
        }

        if (config_.interleaving) {
            interleaver_.reset(new (interleaver_) packet::Interleaver(
                *pwriter, allocator(),
                config_.fec_writer.n_source_packets
                    + config_.fec_writer.n_repair_packets));
            if (!interleaver_ || !interleaver_->valid()) {
                return false;
            }
            pwriter = interleaver_.get();
        }

        fec_encoder_.reset(fec::CodecMap::instance().new_encoder(
                               config_.fec_encoder, byte_buffer_factory_, allocator()),
                           allocator());
        if (!fec_encoder_) {
            return false;
        }

        fec_writer_.reset(new (fec_writer_) fec::Writer(
            config_.fec_writer, config_.fec_encoder.scheme, *fec_encoder_, *pwriter,
            source_endpoint_->composer(), repair_endpoint_->composer(), packet_factory_,
            byte_buffer_factory_, allocator()));
        if (!fec_writer_ || !fec_writer_->valid()) {
            return false;
        }
        pwriter = fec_writer_.get();
    }

    payload_encoder_.reset(format->new_encoder(allocator()), allocator());
    if (!payload_encoder_) {
        return false;
    }

    packetizer_.reset(new (packetizer_) audio::Packetizer(
        *pwriter, source_endpoint_->composer(), *payload_encoder_, packet_factory_,
        byte_buffer_factory_, config_.packet_length,
        audio::SampleSpec(format->sample_spec.sample_rate(),
                          config_.input_sample_spec.channel_mask()),
        config_.payload_type));
    if (!packetizer_ || !packetizer_->valid()) {
        return false;
    }

    audio::IWriter* awriter = packetizer_.get();

    if (config_.resampling
        && config_.input_sample_spec.sample_rate() != format->sample_spec.sample_rate()) {
        if (config_.poisoning) {
            resampler_poisoner_.reset(new (resampler_poisoner_)
                                          audio::PoisonWriter(*awriter));
            if (!resampler_poisoner_) {
                return false;
            }
            awriter = resampler_poisoner_.get();
        }

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
                             config_.resampler_backend, allocator(),
                             sample_buffer_factory_, config_.resampler_profile,
                             config_.internal_frame_length, config_.input_sample_spec),
                         allocator());

        if (!resampler_) {
            return false;
        }

        resampler_writer_.reset(new (resampler_writer_) audio::ResamplerWriter(
            *awriter, *resampler_, sample_buffer_factory_, config_.internal_frame_length,
            config_.input_sample_spec));

        if (!resampler_writer_ || !resampler_writer_->valid()) {
            return false;
        }
        if (!resampler_writer_->set_scaling(config_.input_sample_spec.sample_rate(),
                                            format->sample_spec.sample_rate(), 1.0f)) {
            return false;
        }
        awriter = resampler_writer_.get();
    }

    audio_writer_ = awriter;

    return true;
}

bool SenderEndpointSet::create_control_pipeline_() {
    roc_panic_if(rtcp_session_);
    roc_panic_if(!control_endpoint_);

    rtcp_composer_.reset(new (rtcp_composer_) rtcp::Composer());
    if (!rtcp_composer_) {
        return false;
    }

    rtcp_session_.reset(new (rtcp_session_) rtcp::Session(
        NULL, this, &control_endpoint_->writer(), *rtcp_composer_, packet_factory_,
        byte_buffer_factory_));
    if (!rtcp_session_ || !rtcp_session_->valid()) {
        return false;
    }

    return true;
}

} // namespace pipeline
} // namespace roc
