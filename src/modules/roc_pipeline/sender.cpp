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

#include "roc_pipeline/sender.h"

namespace roc {
namespace pipeline {

Sender::Sender(audio::ISampleBufferReader& audio_reader,
               datagram::IDatagramWriter& datagram_writer,
               datagram::IDatagramComposer& datagram_composer,
               const SenderConfig& config)
    : config_(config)
    , rtp_composer_(*config_.rtp_audio_packet_pool,
                    *config_.rtp_container_packet_pool,
                    *config_.byte_buffer_composer)
    , audio_composer_(NULL)
    , fec_repair_composer_(NULL)
    , packet_sender_(datagram_writer, datagram_composer)
    , audio_reader_(audio_reader)
    , audio_writer_(NULL)
    , datagram_writer_(datagram_writer) {
}

void Sender::set_audio_port(const datagram::Address& source,
                            const datagram::Address& destination,
                            Protocol proto) {
    switch (proto) {
    case Proto_RTP:
        audio_composer_ = &rtp_composer_;
        break;
    }

    if (!audio_composer_) {
        roc_panic("receiver: bad protocol number %d for audio port", (int)proto);
    }

    // TODO: fec_source_composer_

    // TODO: add multiple ports support to PacketSender
    packet_sender_.set_sender(source);
    packet_sender_.set_receiver(destination);
}

void Sender::set_repair_port(const datagram::Address& source,
                             const datagram::Address& destination,
                             Protocol proto) {
    switch (proto) {
    case Proto_RTP:
        fec_repair_composer_ = &rtp_composer_; // FIXME
        break;
    }

    if (!fec_repair_composer_) {
        roc_panic("receiver: bad protocol number %d for repair port", (int)proto);
    }

    // TODO: add multiple ports support to PacketSender
    packet_sender_.set_sender(source);
    packet_sender_.set_receiver(destination);
}

void Sender::run() {
    roc_log(LogInfo, "sender: starting thread");

    for (;;) {
        if (!tick()) {
            break;
        }
    }

    roc_log(LogInfo, "sender: finishing thread");

    flush();

    // Write EOF.
    datagram_writer_.write(NULL);
}

bool Sender::tick() {
    if (!audio_writer_) {
        if (!(audio_writer_ = make_audio_writer_())) {
            roc_panic("sender: can't create audio writer");
        }
    }

    audio::ISampleBufferConstSlice buffer = audio_reader_.read();

    if (buffer) {
        audio_writer_->write(buffer);
    } else {
        roc_log(LogInfo, "sender: audio reader returned null");
    }

    return (bool)buffer;
}

void Sender::flush() {
    if (splitter_) {
        splitter_->flush();
    }

    if (interleaver_) {
        interleaver_->flush();
    }
}

audio::ISampleBufferWriter* Sender::make_audio_writer_() {
    if (!audio_composer_) {
        roc_panic("sender: audio composer not set, forgot set_audio_port()?");
    }

    packet::IPacketWriter* packet_writer = make_packet_writer_();
    roc_panic_if(!packet_writer);

    audio::ISampleBufferWriter* audio_writer = new (splitter_)
        audio::Splitter(*packet_writer, *audio_composer_, config_.samples_per_packet,
                        config_.channels, config_.sample_rate);

    if (config_.options & EnableTiming) {
        audio_writer = new (timed_writer_)
            audio::TimedWriter(*audio_writer, config_.channels, config_.sample_rate);
    }

    return audio_writer;
}

packet::IPacketWriter* Sender::make_packet_writer_() {
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
packet::IPacketWriter* Sender::make_fec_encoder_(packet::IPacketWriter* packet_writer) {

    if (!fec_repair_composer_) {
        roc_panic("sender: fec repair composer not set, forgot set_audio_port()?");
    }

    new (fec_ldpc_encoder_) fec::OFBlockEncoder(config_.fec,
                                                    *config_.byte_buffer_composer);

    return new (fec_encoder_)
        fec::Encoder(*fec_ldpc_encoder_, *packet_writer, *fec_repair_composer_);
}
#else
packet::IPacketWriter* Sender::make_fec_encoder_(packet::IPacketWriter* packet_writer) {
    roc_log(LogError, "sender: OpenFEC support not enabled, disabling fec encoder");
    return packet_writer;
}
#endif

} // namespace pipeline
} // namespace roc
