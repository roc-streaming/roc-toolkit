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

Session::Session(core::IPool<Session>& pool)
    : BasicSession()
    , pool_(pool) {
}

void Session::free() {
    pool_.destroy(*this);
}

packet::IPacketConstWriter* Session::make_packet_writer() {
    return &router;
}

packet::IPacketReader* Session::make_packet_reader() {
    packet::IPacketReader* packet_reader =
        new (audio_packet_queue) packet::PacketQueue(config().max_session_packets);

    router.add_route(packet::IAudioPacket::Type, *audio_packet_queue);

    packet_reader = new (delayer) audio::Delayer(*packet_reader, config().latency);

    packet_reader = new (watchdog) audio::Watchdog(*packet_reader, config().timeout);

    renderer.add_tuner(*watchdog);

    if (config().options & EnableLDPC) {
        packet_reader = make_fec_decoder(packet_reader);
    }

    return packet_reader;
}

audio::IRenderer* Session::make_audio_renderer() {
    packet::IPacketReader* packet_reader = make_packet_reader();
    if (!packet_reader) {
        roc_panic("session: make_packet_reader() returned null");
    }

    if (config().options & EnableResampling) {
        packet_reader = new (scaler) audio::Scaler(*packet_reader, *audio_packet_queue);

        renderer.add_tuner(*scaler);
    }

    audio::IAudioPacketReader* audio_packet_reader =
        new (chanalyzer) audio::Chanalyzer(*packet_reader, config().channels);

    for (packet::channel_t ch = 0; ch < MaxChannels; ch++) {
        if ((config().channels & (1 << ch)) == 0) {
            continue;
        }

        audio::IStreamReader* stream_reader = make_stream_reader(audio_packet_reader, ch);
        if (!stream_reader) {
            roc_panic("session: make_stream_reader() returned null");
        }

        renderer.set_reader(ch, *stream_reader);
    }

    return &renderer;
}

audio::IStreamReader*
Session::make_stream_reader(audio::IAudioPacketReader* audio_packet_reader,
                            packet::channel_t ch) {
    //
    audio::IStreamReader* stream_reader = new (streamers[ch])
        audio::Streamer(*audio_packet_reader, ch, config().options & EnableBeep);

    if (config().options & EnableResampling) {
        roc_panic_if_not(scaler);

        stream_reader = new (resamplers[ch])
            audio::Resampler(*stream_reader, *config().sample_buffer_composer);

        scaler->add_resampler(*resamplers[ch]);
    }

    return stream_reader;
}

#ifdef ROC_TARGET_OPENFEC

packet::IPacketReader* Session::make_fec_decoder(packet::IPacketReader* packet_reader) {
    //
    new (fec_packet_queue) packet::PacketQueue(config().max_session_packets);
    new (fec_ldpc_decoder) fec::LDPC_BlockDecoder(*config().byte_buffer_composer);

    router.add_route(packet::IFECPacket::Type, *fec_packet_queue);

    return new (fec_decoder) fec::Decoder(*fec_ldpc_decoder, *packet_reader,
                                          *fec_packet_queue, packet_parser());
}

#else

packet::IPacketReader* Session::make_fec_decoder(packet::IPacketReader* packet_reader) {
    roc_log(LOG_ERROR, "session: OpenFEC support not enabled, disabling fec decoder");
    return packet_reader;
}

#endif

} // namespace pipeline
} // namespace roc
