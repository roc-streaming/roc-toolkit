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
#include "roc_datagram/address_to_str.h"

#include "roc_pipeline/basic_server.h"

namespace roc {
namespace pipeline {

BasicServer::BasicServer(const ServerConfig& cfg)
    : config_(cfg)
    , n_channels_(packet::num_channels(config_.channels))
    , datagram_reader_(NULL)
    , audio_sink_(NULL)
    , audio_reader_(NULL)
    , audio_writer_(NULL) {
    //
    if (n_channels_ == 0) {
        roc_panic("server: channel mask is zero");
    }

    if (config_.samples_per_tick == 0) {
        roc_panic("server: # of samples per tick is zero");
    }

    if (!config_.byte_buffer_composer) {
        roc_panic("server: byte buffer composer is null");
    }

    if (!config_.sample_buffer_composer) {
        roc_panic("server: sample buffer composer is null");
    }

    if (!config_.session_composer) {
        roc_panic("server: session composer is null");
    }
}

BasicServer::~BasicServer() {
    if (sessions_.size() != 0) {
        roc_panic("server: derived class didn't call destroy_sessions() in destructor");
    }
}

const ServerConfig& BasicServer::config() const {
    return config_;
}

void BasicServer::destroy_sessions() {
    roc_log(LOG_DEBUG, "server: destroying all sessions");

    if (!audio_sink_) {
        roc_panic_if(sessions_.size() != 0);
        return;
    }

    BasicSessionPtr next_session;

    for (BasicSessionPtr session = sessions_.front(); session; session = next_session) {
        next_session = sessions_.next(*session);
        sessions_.remove(*session);

        session->detach(*audio_sink_);
    }
}

size_t BasicServer::num_sessions() const {
    return sessions_.size();
}

void BasicServer::add_port(const datagram::Address& address,
                           packet::IPacketParser& parser) {
    roc_panic_if(&parser == NULL);

    Port port;
    port.address = address;
    port.parser = &parser;

    ports_.append(port);
}

void BasicServer::run() {
    roc_log(LOG_DEBUG, "server: starting thread");

    const size_t n_datagrams = config_.max_sessions * config_.max_session_packets;
    const size_t n_buffers = 1;
    const size_t n_samples = config_.samples_per_tick;

    while (!stop_) {
        if (!tick(n_datagrams, n_buffers, n_samples)) {
            break;
        }
    }

    roc_log(LOG_DEBUG, "server: terminating thread");
}

bool BasicServer::tick(size_t n_datagrams, size_t n_buffers, size_t n_samples) {
    make_pipeline_();

    fetch_datagrams_(n_datagrams);
    update_sessions_();

    return generate_audio_(n_buffers, n_samples);
}

void BasicServer::stop() {
    stop_ = true;
}

void BasicServer::make_pipeline_() {
    if (!datagram_reader_) {
        if (!(datagram_reader_ = make_datagram_reader())) {
            roc_panic("server: make_datagram_reader() returned null");
        }
    }

    if (!audio_sink_) {
        if (!(audio_sink_ = make_audio_sink())) {
            roc_panic("server: make_audio_sink() returned null");
        }
    }

    if (!audio_reader_) {
        if (!(audio_reader_ = make_audio_reader())) {
            roc_panic("server: make_audio_reader() returned null");
        }
    }

    if (!audio_writer_) {
        if (!(audio_writer_ = make_audio_writer())) {
            roc_panic("server: make_audio_writer() returned null");
        }
    }
}

void BasicServer::fetch_datagrams_(size_t n_datagrams) {
    for (size_t n = 0; n < n_datagrams; n++) {
        datagram::IDatagramConstPtr dgm = datagram_reader_->read();
        if (!dgm) {
            break;
        }

        if (find_session_and_store_(*dgm)) {
            continue;
        }

        if (create_session_and_store_(*dgm)) {
            continue;
        }

        // datagram is dropped
    }
}

bool BasicServer::find_session_and_store_(const datagram::IDatagram& dgm) {
    for (BasicSessionPtr session = sessions_.front(); session;
         session = sessions_.next(*session)) {
        if (session->address() == dgm.sender()) {
            session->store(dgm);
            return true;
        }
    }

    return false;
}

bool BasicServer::create_session_and_store_(const datagram::IDatagram& dgm) {
    if (sessions_.size() >= config_.max_sessions) {
        roc_log(LOG_DEBUG, "server: dropping datagram: "
                           "maximum number of session limit reached (%u sessions)",
                (unsigned)sessions_.size());
        return false;
    }

    const Port* port = find_port_(dgm.receiver());
    if (port == NULL) {
        roc_log(LOG_TRACE, "server: dropping datagram: no parser for %s",
                datagram::address_to_str(dgm.receiver()).c_str());
        return false;
    }

    roc_log(LOG_DEBUG, "server: creating session %s",
            datagram::address_to_str(dgm.sender()).c_str());

    BasicSessionPtr session = config_.session_composer->compose();
    if (!session) {
        roc_log(LOG_DEBUG, "server: can't get session from pool");
        return false;
    }

    session->set_address(dgm.sender());
    session->set_parser(*port->parser);
    session->set_config(config_);

    if (!session->store(dgm)) {
        roc_log(LOG_TRACE, "server: ignoring session: can't store first datagram");
        return false;
    }

    session->attach(*audio_sink_);
    sessions_.append(*session);

    return true;
}

const BasicServer::Port* BasicServer::find_port_(const datagram::Address& address) {
    for (size_t n = 0; n < ports_.size(); n++) {
        if (ports_[n].address == address) {
            return &ports_[n];
        }
    }

    return NULL;
}

void BasicServer::update_sessions_() {
    BasicSessionPtr next_session;

    for (BasicSessionPtr session = sessions_.front(); session; session = next_session) {
        next_session = sessions_.next(*session);

        if (!session->update()) {
            roc_log(LOG_DEBUG, "server: removing session %s",
                    datagram::address_to_str(session->address()).c_str());

            session->detach(*audio_sink_);
            sessions_.remove(*session);
        }
    }
}

bool BasicServer::generate_audio_(size_t n_buffers, size_t n_samples) {
    roc_panic_if(n_samples * n_channels_ == 0);

    for (size_t n = 0; n < n_buffers; n++) {
        audio::ISampleBufferPtr buffer = config_.sample_buffer_composer->compose();
        if (!buffer) {
            roc_log(LOG_ERROR, "server: can't compose sample buffer");
            return false;
        }

        buffer->set_size(n_samples * n_channels_);

        audio_reader_->read(*buffer);
        audio_writer_->write(*buffer);
    }

    return true;
}

} // namespace pipeline
} // namespace roc
