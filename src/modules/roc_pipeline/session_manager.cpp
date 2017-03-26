/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_datagram/address_to_str.h"

#include "roc_pipeline/session_manager.h"

namespace roc {
namespace pipeline {

SessionManager::SessionManager(const ServerConfig& config, audio::ISink& sink)
    : config_(config)
    , audio_sink_(sink) {
}

SessionManager::~SessionManager() {
    if (sessions_.size() != 0) {
        destroy_sessions_();
    }
}

size_t SessionManager::num_sessions() const {
    return sessions_.size();
}

void SessionManager::add_port(const datagram::Address& address,
                              packet::IPacketParser& parser) {
    roc_panic_if((const void*)&parser == NULL);

    Port port;
    port.address = address;
    port.parser = &parser;

    ports_.append(port);
}

bool SessionManager::route(const datagram::IDatagram& dgm) {
    const Port* port = find_port_(dgm.receiver());
    if (port == NULL) {
        roc_log(LogDebug, "session manager: dropping datagram: no parser for %s",
                datagram::address_to_str(dgm.receiver()).c_str());
        return false;
    }

    packet::IPacketConstPtr packet = port->parser->parse(dgm.buffer());
    if (!packet) {
        roc_log(LogDebug, "session manager: dropping datagram: can't parse");
        return false;
    }

    if (find_session_and_store_(dgm, packet)) {
        return true;
    }

    if (create_session_and_store_(dgm, packet, *port->parser)) {
        return true;
    }

    return false;
}

bool SessionManager::update() {
    SessionPtr next_session;

    for (SessionPtr session = sessions_.front(); session; session = next_session) {
        next_session = sessions_.next(*session);

        if (!session->update()) {
            roc_log(LogInfo, "session manager: removing session %s",
                    datagram::address_to_str(session->sender()).c_str());

            session->detach(audio_sink_);
            sessions_.remove(*session);

            if ((config_.options & EnableOneshot) && sessions_.size() == 0) {
                return false;
            }
        }
    }

    return true;
}

void SessionManager::destroy_sessions_() {
    roc_log(LogInfo, "session manager: destroying %u sessions",
            (unsigned)sessions_.size());

    SessionPtr next_session;

    for (SessionPtr session = sessions_.front(); session; session = next_session) {
        next_session = sessions_.next(*session);
        sessions_.remove(*session);

        session->detach(audio_sink_);
    }
}

bool SessionManager::find_session_and_store_(const datagram::IDatagram& dgm,
                                             const packet::IPacketConstPtr& packet) {
    for (SessionPtr session = sessions_.front(); session;
         session = sessions_.next(*session)) {
        if (session->may_route(dgm, packet)) {
            session->route(packet);
            return true;
        }
    }

    for (SessionPtr session = sessions_.front(); session;
         session = sessions_.next(*session)) {
        if (session->may_autodetect_route(dgm, packet)) {
            session->route(packet);
            return true;
        }
    }

    return false;
}

bool SessionManager::create_session_and_store_(const datagram::IDatagram& dgm,
                                               const packet::IPacketConstPtr& packet,
                                               packet::IPacketParser& parser) {
    if (sessions_.size() >= config_.max_sessions) {
        roc_log(LogInfo, "session manager: dropping datagram:"
                         " maximum number of session limit reached (%u sessions)",
                (unsigned)sessions_.size());
        return false;
    }

    roc_log(LogInfo, "session manager: creating session %s",
            datagram::address_to_str(dgm.sender()).c_str());

    SessionPtr session = new (*config_.session_pool)
        Session(config_, dgm.sender(), dgm.receiver(), parser);

    if (!session) {
        roc_log(LogInfo, "session manager: can't get session from pool");
        return false;
    }

    if (!session->may_autodetect_route(dgm, packet)) {
        roc_log(LogInfo, "session manager: can't route packet to new session");
        return false;
    }

    session->route(packet);
    session->attach(audio_sink_);
    sessions_.append(*session);

    return true;
}

const SessionManager::Port* SessionManager::find_port_(const datagram::Address& address) {
    for (size_t n = 0; n < ports_.size(); n++) {
        if (ports_[n].address == address) {
            return &ports_[n];
        }
    }

    return NULL;
}

} // namespace pipeline
} // namespace roc
