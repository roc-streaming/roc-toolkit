/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/session_manager.h
//! @brief Session manager.

#ifndef ROC_PIPELINE_SESSION_MANAGER_H_
#define ROC_PIPELINE_SESSION_MANAGER_H_

#include "roc_audio/isink.h"
#include "roc_config/config.h"
#include "roc_core/array.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_datagram/idatagram.h"
#include "roc_packet/ipacket_parser.h"

#include "roc_pipeline/config.h"
#include "roc_pipeline/session.h"

namespace roc {
namespace pipeline {

//! Session manager.
//! @remarks
//!  Maintains list of active sessions and routes incoming datagrams
//!  to them.
class SessionManager : public core::NonCopyable<> {
public:
    //! Initialize session manager.
    SessionManager(const ReceiverConfig& config, audio::ISink& sink);

    ~SessionManager();

    //! Get number of active sessions.
    size_t num_sessions() const;

    //! Register port.
    void add_port(const datagram::Address&, packet::IPacketParser&);

    //! Route datagram to proper session.
    //! @returns false if datagram was dropped.
    bool route(const datagram::IDatagram&);

    //! Update sessions.
    //! @returns false if receiver should be terminated.
    bool update();

private:
    enum { MaxPorts = ROC_CONFIG_MAX_PORTS };

    struct Port {
        datagram::Address address;
        packet::IPacketParser* parser;

        Port()
            : parser(NULL) {
        }
    };

    void destroy_sessions_();

    bool find_session_and_store_(const datagram::IDatagram&,
                                 const packet::IPacketConstPtr&);

    bool create_session_and_store_(const datagram::IDatagram&,
                                   const packet::IPacketConstPtr&,
                                   packet::IPacketParser&);

    const Port* find_port_(const datagram::Address&);

    const ReceiverConfig config_;
    audio::ISink& audio_sink_;

    core::Array<Port, MaxPorts> ports_;
    core::List<Session> sessions_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SESSION_MANAGER_H_
