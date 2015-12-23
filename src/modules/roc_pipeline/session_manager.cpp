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
    roc_panic_if(&parser == NULL);

    Port port;
    port.address = address;
    port.parser = &parser;

    ports_.append(port);
}

bool SessionManager::store(const datagram::IDatagram& dgm) {
    if (find_session_and_store_(dgm)) {
        return true;
    }

    if (create_session_and_store_(dgm)) {
        return true;
    }

    return false;
}

bool SessionManager::update() {
    SessionPtr next_session;

    for (SessionPtr session = sessions_.front(); session; session = next_session) {
        next_session = sessions_.next(*session);

        if (!session->update()) {
            roc_log(LOG_DEBUG, "session manager: removing session %s",
                    datagram::address_to_str(session->address()).c_str());

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
    roc_log(LOG_DEBUG, "session manager: destroying %u sessions",
            (unsigned)sessions_.size());

    SessionPtr next_session;

    for (SessionPtr session = sessions_.front(); session; session = next_session) {
        next_session = sessions_.next(*session);
        sessions_.remove(*session);

        session->detach(audio_sink_);
    }
}

bool SessionManager::find_session_and_store_(const datagram::IDatagram& dgm) {
    for (SessionPtr session = sessions_.front(); session;
         session = sessions_.next(*session)) {
        if (session->address() == dgm.sender()) {
            session->store(dgm);
            return true;
        }
    }

    return false;
}

bool SessionManager::create_session_and_store_(const datagram::IDatagram& dgm) {
    if (sessions_.size() >= config_.max_sessions) {
        roc_log(LOG_DEBUG, "session manager: dropping datagram:"
                           " maximum number of session limit reached (%u sessions)",
                (unsigned)sessions_.size());
        return false;
    }

    const Port* port = find_port_(dgm.receiver());
    if (port == NULL) {
        roc_log(LOG_TRACE, "session manager: dropping datagram: no parser for %s",
                datagram::address_to_str(dgm.receiver()).c_str());
        return false;
    }

    roc_log(LOG_DEBUG, "session manager: creating session %s",
            datagram::address_to_str(dgm.sender()).c_str());

    SessionPtr session =
        new (*config_.session_pool) Session(config_, dgm.sender(), *port->parser);

    if (!session) {
        roc_log(LOG_DEBUG, "session manager: can't get session from pool");
        return false;
    }

    if (!session->store(dgm)) {
        roc_log(LOG_TRACE, "session manager:"
                " ignoring session: can't store first datagram");
        return false;
    }

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
