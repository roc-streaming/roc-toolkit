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

#include "roc_pipeline/basic_session.h"

namespace roc {
namespace pipeline {

BasicSession::BasicSession()
    : packet_parser_(NULL)
    , packet_writer_(NULL)
    , audio_renderer_(NULL)
    , config_(NULL) {
}

BasicSession::~BasicSession() {
}

void BasicSession::set_address(const datagram::Address& addr) {
    address_ = addr;
}

void BasicSession::set_parser(packet::IPacketParser& parser) {
    packet_parser_ = &parser;
}

void BasicSession::set_config(const ServerConfig& cfg) {
    if (config_) {
        roc_panic("session: can't call set_config() more than once");
    }

    config_ = &cfg;

    if (!(packet_writer_ = make_packet_writer())) {
        roc_panic("client: make_packet_writer() returned null");
    }

    if (!(audio_renderer_ = make_audio_renderer())) {
        roc_panic("client: make_audio_renderer() returned null");
    }
}

const datagram::Address& BasicSession::address() const {
    return address_;
}

const ServerConfig& BasicSession::config() const {
    if (!config_) {
        roc_panic("session: set_config() was not called");
    }

    return *config_;
}

packet::IPacketParser& BasicSession::packet_parser() const {
    if (!packet_parser_) {
        roc_panic("session: set_parser() was not called");
    }

    return *packet_parser_;
}

bool BasicSession::store(const datagram::IDatagram& dgm) {
    if (!packet_parser_) {
        roc_panic("session: set_parser() was not called");
    }

    if (!packet_writer_) {
        roc_panic("session: set_config() was not called");
    }

    packet::IPacketConstPtr packet = packet_parser_->parse(dgm.buffer());
    if (!packet) {
        roc_log(LOG_TRACE, "session: dropping datagram: can't parse");
        return false;
    }

    packet_writer_->write(packet);
    return true;
}

bool BasicSession::update() {
    if (!audio_renderer_) {
        roc_panic("session: set_config() was not called");
    }

    return audio_renderer_->update();
}

void BasicSession::attach(audio::ISink& sink) {
    if (!audio_renderer_) {
        roc_panic("session: set_config() was not called");
    }

    audio_renderer_->attach(sink);
}

void BasicSession::detach(audio::ISink& sink) {
    if (!audio_renderer_) {
        roc_panic("session: set_config() was not called");
    }

    audio_renderer_->detach(sink);
}

} // namespace pipeline
} // namespace roc
