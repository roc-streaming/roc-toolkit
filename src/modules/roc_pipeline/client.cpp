/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"

#include "roc_pipeline/client.h"

namespace roc {
namespace pipeline {

Client::Client(audio::ISampleBufferReader& audio_reader,
               datagram::IDatagramWriter& datagram_writer,
               datagram::IDatagramComposer& datagram_composer,
               packet::IPacketComposer& packet_composer,
               const ClientConfig& config)
    : config_(config)
    , packet_sender_(datagram_writer, datagram_composer)
    , packet_composer_(packet_composer)
    , audio_reader_(audio_reader)
    , audio_writer_(*make_audio_writer_())
    , datagram_writer_(datagram_writer) {
}

void Client::set_sender(const datagram::Address& address) {
    packet_sender_.set_sender(address);
}

void Client::set_receiver(const datagram::Address& address) {
    packet_sender_.set_receiver(address);
}

void Client::run() {
    roc_log(LogInfo, "client: starting thread");

    for (;;) {
        if (!tick()) {
            break;
        }
    }

    roc_log(LogInfo, "client: finishing thread");

    flush();

    datagram_writer_.write(NULL);
}

bool Client::tick() {
    audio::ISampleBufferConstSlice buffer = audio_reader_.read();

    if (buffer) {
        audio_writer_.write(buffer);
    } else {
        roc_log(LogInfo, "client: audio reader returned null");
    }

    return (bool)buffer;
}

void Client::flush() {
    if (splitter_) {
        splitter_->flush();
    }

    if (interleaver_) {
        interleaver_->flush();
    }
}

audio::ISampleBufferWriter* Client::make_audio_writer_() {
    packet::IPacketWriter* packet_writer = make_packet_writer_();
    roc_panic_if(!packet_writer);

    audio::ISampleBufferWriter* audio_writer = new (splitter_)
        audio::Splitter(*packet_writer, packet_composer_, config_.samples_per_packet,
                        config_.channels, config_.sample_rate);

    if (config_.options & EnableTiming) {
        audio_writer = new (timed_writer_)
            audio::TimedWriter(*audio_writer, config_.channels, config_.sample_rate);
    }

    return audio_writer;
}

packet::IPacketWriter* Client::make_packet_writer_() {
    packet::IPacketWriter* packet_writer = &packet_sender_;

    if (config_.random_loss_rate || config_.random_delay_rate) {
        packet_writer = new (spoiler_) packet::Spoiler(*packet_writer);

        spoiler_->set_random_loss(config_.random_loss_rate);
        spoiler_->set_random_delay(config_.random_delay_rate, config_.random_delay_time);
    }

    if (config_.options & EnableInterleaving) {
        packet_writer = new (interleaver_) packet::Interleaver(
            *packet_writer, ROC_CONFIG_DEFAULT_FEC_BLOCK_REDUNDANT_PACKETS);
    }

    if (config_.fec.codec != fec::NoCodec) {
        packet_writer = make_fec_encoder_(packet_writer);
    }

    return packet_writer;
}

#ifdef ROC_TARGET_OPENFEC
packet::IPacketWriter* Client::make_fec_encoder_(packet::IPacketWriter* packet_writer) {
    new (fec_ldpc_encoder_) fec::OFBlockEncoder(config_.fec,
                                                    *config_.byte_buffer_composer);

    return new (fec_encoder_)
        fec::Encoder(*fec_ldpc_encoder_, *packet_writer, packet_composer_);
}
#else
packet::IPacketWriter* Client::make_fec_encoder_(packet::IPacketWriter* packet_writer) {
    roc_log(LogError, "client: OpenFEC support not enabled, disabling fec encoder");
    return packet_writer;
}
#endif

} // namespace pipeline
} // namespace roc
