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
               const ClientConfig& cfg)
    : BasicClient(cfg, datagram_writer)
    , input_reader(audio_reader)
    , packet_sender(datagram_writer)
    , packet_composer(NULL) {
}

void Client::set_composers(packet::IPacketComposer& pkt_composer,
                           datagram::IDatagramComposer& dgm_composer) {
    packet_composer = &pkt_composer;
    packet_sender.set_composer(dgm_composer);
}

void Client::set_sender(const datagram::Address& address) {
    packet_sender.set_sender(address);
}

void Client::set_receiver(const datagram::Address& address) {
    packet_sender.set_receiver(address);
}

audio::ISampleBufferReader* Client::make_audio_reader() {
    return &input_reader;
}

audio::ISampleBufferWriter* Client::make_audio_writer() {
    packet::IPacketWriter* packet_writer = make_packet_writer();
    if (!packet_writer) {
        roc_panic("client: make_packet_writer() returned null");
    }

    if (!packet_composer) {
        roc_panic("client: set_composers() was not called");
    }

    audio::ISampleBufferWriter* audio_writer = new (splitter) audio::Splitter(
        *packet_writer, *packet_composer, config().samples_per_packet, config().channels);

    if (config().options & EnableTiming) {
        audio_writer = new (timed_writer)
            audio::TimedWriter(*audio_writer, config().channels, config().sample_rate);
    }

    return audio_writer;
}

packet::IPacketWriter* Client::make_packet_writer() {
    packet::IPacketWriter* packet_writer = &packet_sender;

    if (config().random_loss_rate || config().random_delay_rate) {
        packet_writer = new (wrecker) packet::Wrecker(*packet_writer);

        wrecker->set_random_loss(config().random_loss_rate);
        wrecker->set_random_delay(config().random_delay_rate, config().random_delay_time);
    }

    if (config().options & EnableInterleaving) {
        packet_writer = new (interleaver) packet::Interleaver(*packet_writer);
    }

    if (config().options & EnableLDPC) {
        packet_writer = make_fec_encoder(packet_writer);
    }

    return packet_writer;
}

#ifdef ROC_TARGET_OPENFEC

packet::IPacketWriter* Client::make_fec_encoder(packet::IPacketWriter* packet_writer) {
    new (fec_ldpc_encoder) fec::LDPC_BlockEncoder(*config().byte_buffer_composer);

    return new (fec_encoder)
        fec::Encoder(*fec_ldpc_encoder, *packet_writer, *packet_composer);
}

#else

packet::IPacketWriter* Client::make_fec_encoder(packet::IPacketWriter* packet_writer) {
    roc_log(LOG_ERROR, "client: OpenFEC support not enabled, disabling fec encoder");
    return packet_writer;
}

#endif

} // namespace pipeline
} // namespace roc
