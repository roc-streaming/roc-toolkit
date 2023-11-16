/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sdp/media_description.h"

namespace roc {
namespace sdp {

MediaDescription::MediaDescription(core::IArena& arena)
    : core::RefCounted<MediaDescription, core::ArenaAllocation>(arena)
    , payload_ids_(arena)
    , connection_data_(arena) {
    clear();
}

void MediaDescription::clear() {
    payload_ids_.clear();
    connection_data_.clear();
    type_ = MediaType_None;
    port_ = 0;
    nb_ports_ = 0;
    transport_ = MediaTransport_None;
}

MediaType MediaDescription::type() const {
    return type_;
}

int MediaDescription::port() const {
    return port_;
}

int MediaDescription::nb_ports() const {
    return nb_ports_;
}

MediaTransport MediaDescription::transport() const {
    return transport_;
}

unsigned MediaDescription::default_payload_id() const {
    if (payload_ids_.size() == 0) {
        roc_panic(
            "media description: MediaDescription should have at least one payload id.");
    }

    return *payload_ids_.data();
}

size_t MediaDescription::nb_payload_ids() const {
    return payload_ids_.size();
}

unsigned MediaDescription::payload_id(size_t i) const {
    return payload_ids_[i];
}

size_t MediaDescription::nb_connection_data() const {
    return connection_data_.size();
}

const ConnectionData& MediaDescription::connection_data(size_t i) const {
    return connection_data_[i];
}

bool MediaDescription::set_type(MediaType type) {
    type_ = type;
    return true;
}

bool MediaDescription::set_transport(MediaTransport transport) {
    transport_ = transport;
    return true;
}

bool MediaDescription::set_port(long port) {
    if (port < 0 || port > 65535) {
        return false;
    }

    port_ = (int)port;
    return true;
}

bool MediaDescription::set_nb_ports(long nb_ports) {
    if (nb_ports < 0 || nb_ports > 65535) {
        return false;
    }

    nb_ports_ = (int)nb_ports;
    return true;
}

bool MediaDescription::add_payload_id(unsigned payload_id) {
    if (!payload_ids_.push_back(payload_id)) {
        return false;
    }

    return true;
}

bool MediaDescription::add_connection_data(address::AddrFamily addrtype,
                                           const char* str,
                                           size_t str_len) {
    ConnectionData c;

    if (!c.set_connection_address(addrtype, str, str_len)) {
        return false;
    }

    if (!connection_data_.push_back(c)) {
        return false;
    }

    return true;
}

} // namespace sdp
} // namespace roc
