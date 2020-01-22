/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_sink.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_pipeline/port_to_str.h"
#include "roc_pipeline/validate_endpoints.h"

namespace roc {
namespace pipeline {

SenderSink::SenderSink(const SenderConfig& config,
                       const PortConfig& source_port_config,
                       packet::IWriter& source_writer,
                       const PortConfig& repair_port_config,
                       packet::IWriter& repair_writer,
                       const fec::CodecMap& codec_map,
                       const rtp::FormatMap& format_map,
                       packet::PacketPool& packet_pool,
                       core::BufferPool<uint8_t>& byte_buffer_pool,
                       core::BufferPool<audio::sample_t>& sample_buffer_pool,
                       core::IAllocator& allocator)
    : audio_writer_(NULL)
    , config_(config)
    , timestamp_(0)
    , num_channels_(packet::num_channels(config.input_channels)) {
    roc_log(LogInfo, "sender: using remote source port %s",
            port_to_str(source_port_config).c_str());
    roc_log(LogInfo, "sender: using remote repair port %s",
            port_to_str(repair_port_config).c_str());

    if (!validate_transport_endpoint_pair(config.fec_encoder.scheme,
                                          source_port_config.protocol,
                                          repair_port_config.protocol)) {
        return;
    }

    const rtp::Format* format = format_map.format(config.payload_type);
    if (!format) {
        return;
    }

    if (config.timing) {
        ticker_.reset(new (allocator) core::Ticker(config.input_sample_rate), allocator);
        if (!ticker_) {
            return;
        }
    }

    source_port_.reset(new (allocator)
                           SenderPort(source_port_config, source_writer, allocator),
                       allocator);
    if (!source_port_ || !source_port_->valid()) {
        return;
    }

    router_.reset(new (allocator) packet::Router(allocator, 2), allocator);
    if (!router_ || !router_->valid()) {
        return;
    }
    packet::IWriter* pwriter = router_.get();

    if (!router_->add_route(*source_port_, packet::Packet::FlagAudio)) {
        return;
    }

    if (config.fec_encoder.scheme != packet::FEC_None) {
        repair_port_.reset(new (allocator)
                               SenderPort(repair_port_config, repair_writer, allocator),
                           allocator);
        if (!repair_port_ || !repair_port_->valid()) {
            return;
        }

        if (!router_->add_route(*repair_port_, packet::Packet::FlagRepair)) {
            return;
        }

        if (config.interleaving) {
            interleaver_.reset(new (allocator) packet::Interleaver(
                                   *pwriter, allocator,
                                   config.fec_writer.n_source_packets
                                       + config.fec_writer.n_repair_packets),
                               allocator);
            if (!interleaver_ || !interleaver_->valid()) {
                return;
            }
            pwriter = interleaver_.get();
        }

        fec_encoder_.reset(
            codec_map.new_encoder(config.fec_encoder, byte_buffer_pool, allocator),
            allocator);
        if (!fec_encoder_) {
            return;
        }

        fec_writer_.reset(
            new (allocator)
                fec::Writer(config.fec_writer, config.fec_encoder.scheme, *fec_encoder_,
                            *pwriter, source_port_->composer(), repair_port_->composer(),
                            packet_pool, byte_buffer_pool, allocator),
            allocator);
        if (!fec_writer_ || !fec_writer_->valid()) {
            return;
        }
        pwriter = fec_writer_.get();
    }

    payload_encoder_.reset(format->new_encoder(allocator), allocator);
    if (!payload_encoder_) {
        return;
    }

    packetizer_.reset(new (allocator) audio::Packetizer(
                          *pwriter, source_port_->composer(), *payload_encoder_,
                          packet_pool, byte_buffer_pool, config.input_channels,
                          config.packet_length, format->sample_rate, config.payload_type),
                      allocator);
    if (!packetizer_) {
        return;
    }

    audio::IWriter* awriter = packetizer_.get();

    if (config.resampling && config.input_sample_rate != format->sample_rate) {
        if (config.poisoning) {
            resampler_poisoner_.reset(new (allocator) audio::PoisonWriter(*awriter),
                                      allocator);
            if (!resampler_poisoner_) {
                return;
            }
            awriter = resampler_poisoner_.get();
        }

        audio::ResamplerMap resampler_map;

        resampler_.reset(resampler_map.new_resampler(
                             config.resampler_backend, allocator, config.resampler,
                             config.input_channels, config.internal_frame_size),
                         allocator);

        if (!resampler_) {
            return;
        }

        resampler_writer_.reset(
            new (allocator) audio::ResamplerWriter(
                *awriter, *resampler_, sample_buffer_pool, config.internal_frame_size),
            allocator);

        if (!resampler_writer_ || !resampler_writer_->valid()) {
            return;
        }
        if (!resampler_writer_->set_scaling(float(config.input_sample_rate)
                                            / format->sample_rate)) {
            return;
        }
        awriter = resampler_writer_.get();
    }

    if (config.poisoning) {
        pipeline_poisoner_.reset(new (allocator) audio::PoisonWriter(*awriter),
                                 allocator);
        if (!pipeline_poisoner_) {
            return;
        }
        awriter = pipeline_poisoner_.get();
    }

    audio_writer_ = awriter;
}

bool SenderSink::valid() {
    return audio_writer_;
}

size_t SenderSink::sample_rate() const {
    return config_.input_sample_rate;
}

size_t SenderSink::num_channels() const {
    return num_channels_;
}

bool SenderSink::has_clock() const {
    return config_.timing;
}

void SenderSink::write(audio::Frame& frame) {
    roc_panic_if(!valid());

    if (ticker_) {
        ticker_->wait(timestamp_);
    }

    audio_writer_->write(frame);
    timestamp_ += frame.size() / num_channels_;
}

} // namespace pipeline
} // namespace roc
