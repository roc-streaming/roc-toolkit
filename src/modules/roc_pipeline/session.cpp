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

#include "roc_pipeline/session.h"
#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {

Session::Session(const ServerConfig& config,
                 const datagram::Address& send_addr,
                 const datagram::Address& recv_addr,
                 packet::IPacketParser& parser)
    : config_(config)
    , send_addr_(send_addr)
    , recv_addr_(recv_addr)
    , packet_parser_(parser)
    , streamers_(MaxChannels)
    , resamplers_(MaxChannels)
    , readers_(MaxChannels) {
    //
    if (!config_.session_pool) {
        roc_panic("session: session pool is null");
    }

    make_pipeline_();
}

void Session::free() {
    config_.session_pool->destroy(*this);
}

const datagram::Address& Session::sender() const {
    return send_addr_;
}

bool Session::may_route(const datagram::IDatagram& dgm,
                        const packet::IPacketConstPtr& packet) const {
    if (dgm.sender() != send_addr_ || dgm.receiver() != recv_addr_) {
        return false;
    } else {
        return router_.may_route(packet);
    }
}

bool Session::may_autodetect_route(const datagram::IDatagram& dgm,
                                   const packet::IPacketConstPtr& packet) const {
    if (dgm.sender() != send_addr_ || dgm.receiver() != recv_addr_) {
        return false;
    } else {
        return router_.may_autodetect_route(packet);
    }
}

void Session::route(const packet::IPacketConstPtr& packet) {
    router_.write(packet);
}

bool Session::update() {
    packet::IMonitor* monitor = monitors_.front();

    for (; monitor != NULL; monitor = monitors_.next(*monitor)) {
        if (!monitor->update()) {
            roc_log(LogInfo, "session: monitor requested session termination");
            return false;
        }
    }

    return true;
}

void Session::attach(audio::ISink& sink) {
    roc_log(LogDebug, "session: attaching readers to sink");

    for (size_t ch = 0; ch < readers_.size(); ch++) {
        if (readers_[ch]) {
            sink.attach(packet::channel_t(ch), *readers_[ch]);
        }
    }
}

void Session::detach(audio::ISink& sink) {
    roc_log(LogDebug, "session: detaching readers from sink");

    for (size_t ch = 0; ch < readers_.size(); ch++) {
        if (readers_[ch]) {
            sink.detach(packet::channel_t(ch), *readers_[ch]);
        }
    }
}

void Session::make_pipeline_() {
    packet::IPacketReader* packet_reader = make_packet_reader_();
    roc_panic_if(!packet_reader);

    if (config_.options & EnableResampling) {
        packet_reader =
            new (scaler_) audio::Scaler(*packet_reader, *audio_packet_queue_,
                                        (packet::timestamp_t)config_.session_latency);

        monitors_.append(*scaler_);
    }

    new (chanalyzer_) audio::Chanalyzer(*packet_reader, config_.channels);

    for (packet::channel_t ch = 0; ch < MaxChannels; ch++) {
        if ((config_.channels & (1 << ch)) == 0) {
            continue;
        }
        readers_[ch] = make_stream_reader_(chanalyzer_->reader(ch), ch);
        roc_panic_if(!readers_[ch]);
    }
}

audio::IStreamReader* Session::make_stream_reader_(packet::IPacketReader& packet_reader,
                                                   packet::channel_t ch) {
    //
    audio::IStreamReader* stream_reader = new (streamers_[ch])
        audio::Streamer(packet_reader, ch, config_.options & EnableBeep);

    if (config_.options & EnableResampling) {
        roc_panic_if_not(scaler_);

        stream_reader = new (resamplers_[ch])
            audio::Resampler(*stream_reader, *config_.sample_buffer_composer,
                             config_.samples_per_resampler_frame);

        scaler_->add_resampler(*resamplers_[ch]);
    }

    return stream_reader;
}

packet::IPacketReader* Session::make_packet_reader_() {
    packet::IPacketReader* packet_reader =
        new (audio_packet_queue_) packet::PacketQueue(config_.max_session_packets);

    router_.add_route(packet::IAudioPacket::Type, *audio_packet_queue_);

    packet_reader = new (delayer_)
        audio::Delayer(*packet_reader, (packet::timestamp_t)config_.session_latency);

    packet_reader = new (watchdog_) packet::Watchdog(
        *packet_reader, config_.session_timeout / config_.samples_per_tick,
        config_.sample_rate);

    monitors_.append(*watchdog_);

    if (config_.options & EnableFEC) {
        packet_reader = make_fec_decoder_(packet_reader);
    }

    return packet_reader;
}

#ifdef ROC_TARGET_OPENFEC
packet::IPacketReader* Session::make_fec_decoder_(packet::IPacketReader* packet_reader) {
    //
    new (fec_packet_queue_) packet::PacketQueue(config_.max_session_packets);
    new (fec_ldpc_decoder_) fec::LDPC_BlockDecoder(*config_.byte_buffer_composer);

    router_.add_route(packet::IFECPacket::Type, *fec_packet_queue_);

    packet_reader = new (fec_decoder_) fec::Decoder(*fec_ldpc_decoder_, *packet_reader,
                                                    *fec_packet_queue_, packet_parser_);

    packet_reader = new (fec_watchdog_) packet::Watchdog(
        *packet_reader, config_.session_timeout / config_.samples_per_tick,
        config_.sample_rate);

    monitors_.append(*fec_watchdog_);

    return packet_reader;
}
#else
packet::IPacketReader* Session::make_fec_decoder_(packet::IPacketReader* packet_reader) {
    roc_log(LogError, "session: OpenFEC support not enabled, disabling fec decoder");
    (void)packet_parser_;
    return packet_reader;
}
#endif

} // namespace pipeline
} // namespace roc
