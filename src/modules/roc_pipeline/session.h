/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/session.h
//! @brief Session pipeline.

#ifndef ROC_PIPELINE_SESSION_H_
#define ROC_PIPELINE_SESSION_H_

#include "roc_config/config.h"

#include "roc_core/ipool.h"
#include "roc_core/maybe.h"

#include "roc_packet/packet_queue.h"
#include "roc_packet/packet_router.h"

#include "roc_fec/decoder.h"

#ifdef ROC_TARGET_OPENFEC
#include "roc_fec/ldpc_block_decoder.h"
#endif

#include "roc_audio/watchdog.h"
#include "roc_audio/delayer.h"
#include "roc_audio/chanalyzer.h"
#include "roc_audio/streamer.h"
#include "roc_audio/resampler.h"
#include "roc_audio/scaler.h"
#include "roc_audio/renderer.h"

#include "roc_pipeline/basic_session.h"

namespace roc {
namespace pipeline {

//! Session pipeline.
//!
//! @see Server.
class Session : public BasicSession {
public:
    //! Create session.
    Session(core::IPool<Session>&);

protected:
    //! Maximum number of channels.
    static const size_t MaxChannels = ROC_CONFIG_MAX_CHANNELS;

    //! Create packet writer.
    virtual packet::IPacketConstWriter* make_packet_writer();

    //! Create renderer.
    //! @remarks
    //!  - Calls make_packet_reader().
    //!  - Calls make_stream_reader().
    virtual audio::IRenderer* make_audio_renderer();

    //! Create stream reader for given channel.
    virtual audio::IStreamReader* make_stream_reader(audio::IAudioPacketReader*,
                                                     packet::channel_t);

    //! Create packet reader.
    //! @remarks
    //!  Calls make_fec_decoder() if EnableLDPC options is set.
    virtual packet::IPacketReader* make_packet_reader();

    //! Create FEC decoder.
    virtual packet::IPacketReader* make_fec_decoder(packet::IPacketReader*);

    //! Queue for audio packets.
    core::Maybe<packet::PacketQueue> audio_packet_queue;

    //! Queue for FEC packets.
    core::Maybe<packet::PacketQueue> fec_packet_queue;

    //! Inserts initial delay before reading audio packets.
    core::Maybe<audio::Delayer> delayer;

    //! Triggers session termination.
    core::Maybe<audio::Watchdog> watchdog;

#ifdef ROC_TARGET_OPENFEC
    //! FEC block decoder.
    core::Maybe<fec::LDPC_BlockDecoder> fec_ldpc_decoder;

    //! FEC packet decoder.
    core::Maybe<fec::Decoder> fec_decoder;
#endif

    //! Packet multiplexer.
    core::Maybe<audio::Chanalyzer> chanalyzer;

    //! Streamers.
    core::Maybe<audio::Streamer> streamers[MaxChannels];

    //! Resamplers.
    core::Maybe<audio::Resampler> resamplers[MaxChannels];

    //! Scaling tuner.
    core::Maybe<audio::Scaler> scaler;

    //! Packet router.
    packet::PacketRouter router;

    //! Audio renderer.
    audio::Renderer renderer;

private:
    virtual void free();

    core::IPool<Session>& pool_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SESSION_H_
