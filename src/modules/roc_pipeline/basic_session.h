/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/basic_session.h
//! @brief Base class for session pipeline.

#ifndef ROC_PIPELINE_BASIC_SESSION_H_
#define ROC_PIPELINE_BASIC_SESSION_H_

#include "roc_core/refcnt.h"
#include "roc_core/list_node.h"
#include "roc_core/shared_ptr.h"

#include "roc_datagram/idatagram.h"
#include "roc_datagram/address.h"

#include "roc_packet/ipacket_parser.h"
#include "roc_packet/ipacket_writer.h"

#include "roc_audio/irenderer.h"

namespace roc {
namespace pipeline {

struct ServerConfig;
class BasicSession;

//! Base class for session pipeline.
//! @remarks
//!  Session object is created for every client connected to server.
class BasicSession : public core::RefCnt, public core::ListNode {
public:
    //! Set client address.
    void set_address(const datagram::Address&);

    //! Set packet parser.
    void set_parser(packet::IPacketParser&);

    //! Set server configuartion.
    void set_config(const ServerConfig&);

    //! Get client address.
    const datagram::Address& address() const;

    //! Parse datagram and add it to internal storage.
    //! @returns
    //!  true if datagram was successfully parsed and stored.
    bool store(const datagram::IDatagram&);

    //! Update renderer state.
    //! @returns
    //!  false if session is broken and should be terminated.
    bool update();

    //! Attach renderer to audio sink.
    void attach(audio::ISink& sink);

    //! Detach renderer from audio sink.
    void detach(audio::ISink& sink);

protected:
    BasicSession();
    ~BasicSession();

    //! Create packet writer.
    //! @remarks
    //!  Used to store parsed packets.
    virtual packet::IPacketConstWriter* make_packet_writer() = 0;

    //! Create renderer.
    //! @remarks
    //!  Used to update sesssion and attach it to sink.
    virtual audio::IRenderer* make_audio_renderer() = 0;

    //! Get config.
    const ServerConfig& config() const;

    //! Get packet parser.
    packet::IPacketParser& packet_parser() const;

private:
    packet::IPacketParser* packet_parser_;
    packet::IPacketConstWriter* packet_writer_;
    audio::IRenderer* audio_renderer_;

    datagram::Address address_;
    const ServerConfig* config_;
};

//! BasicSession smart pointer.
typedef core::SharedPtr<BasicSession> BasicSessionPtr;

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_BASIC_SESSION_H_
