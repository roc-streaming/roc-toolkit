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

#include "roc_core/array.h"
#include "roc_core/ipool.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/maybe.h"
#include "roc_core/refcnt.h"
#include "roc_core/shared_ptr.h"

#include "roc_datagram/address.h"
#include "roc_datagram/idatagram.h"

#include "roc_packet/imonitor.h"
#include "roc_packet/ipacket_parser.h"
#include "roc_packet/ipacket_writer.h"
#include "roc_packet/packet_queue.h"
#include "roc_packet/packet_router.h"
#include "roc_packet/watchdog.h"

#include "roc_fec/decoder.h"

#ifdef ROC_TARGET_OPENFEC
#include "roc_fec/of_block_decoder.h"
#endif

#include "roc_audio/chanalyzer.h"
#include "roc_audio/delayer.h"
#include "roc_audio/isink.h"
#include "roc_audio/resampler.h"
#include "roc_audio/scaler.h"
#include "roc_audio/streamer.h"

namespace roc {
namespace pipeline {

struct ReceiverConfig;

//! Session pipeline.
//! @remarks
//!  Session object is created in receiver for for every connected sender.
//!
//! @see Receiver.
class Session : public core::RefCnt, public core::ListNode {
public:
    //! Create session.
    Session(const ReceiverConfig& config,
            const datagram::Address& send_addr,
            const datagram::Address& recv_addr,
            packet::IPacketParser& parser);

    //! Get sender address.
    const datagram::Address& sender() const;

    //! Check if packet may be routed to this session.
    bool may_route(const datagram::IDatagram&, const packet::IPacketConstPtr&) const;

    //! Check if there is a new route may be created from packet for with session.
    bool may_autodetect_route(const datagram::IDatagram&,
                              const packet::IPacketConstPtr&) const;

    //! Route packet to a proper queue.
    void route(const packet::IPacketConstPtr&);

    //! Update renderer state.
    //! @returns
    //!  false if session is broken and should be terminated.
    bool update();

    //! Attach renderer to audio sink.
    void attach(audio::ISink& sink);

    //! Detach renderer from audio sink.
    void detach(audio::ISink& sink);

private:
    enum { MaxChannels = ROC_CONFIG_MAX_CHANNELS };

    virtual void free();

    void make_pipeline_();

    audio::IStreamReader* make_stream_reader_(packet::IPacketReader&, packet::channel_t);

    packet::IPacketReader* make_packet_reader_();
    packet::IPacketReader* make_fec_decoder_(packet::IPacketReader*);

    const ReceiverConfig& config_;
    const datagram::Address send_addr_;
    const datagram::Address recv_addr_;
    packet::IPacketParser& packet_parser_;

    core::Maybe<packet::PacketQueue> audio_packet_queue_;
    core::Maybe<packet::PacketQueue> fec_packet_queue_;

    core::Maybe<audio::Delayer> delayer_;
    core::Maybe<packet::Watchdog> watchdog_;

#ifdef ROC_TARGET_OPENFEC
    core::Maybe<fec::OFBlockDecoder> fec_blk_decoder_;
    core::Maybe<fec::Decoder> fec_decoder_;
    core::Maybe<packet::Watchdog> fec_watchdog_;
#endif

    core::Maybe<audio::Chanalyzer> chanalyzer_;
    core::Array<core::Maybe<audio::Streamer>, MaxChannels> streamers_;
    core::Array<core::Maybe<audio::Resampler>, MaxChannels> resamplers_;
    core::Maybe<audio::Scaler> scaler_;
    packet::PacketRouter router_;

    core::List<packet::IMonitor, core::NoOwnership> monitors_;
    core::Array<audio::IStreamReader*, MaxChannels> readers_;
};

//! Session smart pointer.
typedef core::SharedPtr<Session> SessionPtr;

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SESSION_H_
