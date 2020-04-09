/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sdp/media_description.h"

namespace roc {
namespace sdp {

MediaDescription::MediaDescription(core::IAllocator& allocator)
    : media_(allocator)
    , fmts_(allocator)
    , connection_data_(allocator)
    , allocator_(allocator) {
    clear();
}

void MediaDescription::clear() {
    media_.clear();
    fmts_.clear();
    connection_data_.resize(0);
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

MediaProto MediaDescription::proto() const {
    return proto_;
}

const char* MediaDescription::default_fmt() const {
    if (fmts_.size() == 0) {
        return NULL;
    }

    return fmts_.front();
}

bool MediaDescription::set_type(MediaType type) {
    type_ = type;
    return true;
}

bool MediaDescription::set_proto(MediaProto proto) {
    proto_ = proto;
    return true;
}

bool MediaDescription::set_port(int port) {
    port_ = port;
    return true;
}

bool MediaDescription::set_nb_ports(int nb_ports) {
    nb_ports_ = nb_ports;
    return true;
}

bool MediaDescription::add_fmt(const char* str, size_t str_len) {
    core::StringBuffer<> fmt(allocator_);
    if (!fmt.set_buf(str, str_len) || fmt.is_empty()) {
        return false;
    }

    if (!fmts_.push_back(fmt.c_str())) {
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

    if (connection_data_.size() >= connection_data_.max_size()) {
        if (!connection_data_.grow(connection_data_.size() + 1)) {
            return false;
        }
    }
    connection_data_.push_back(c);

    return true;
}

void MediaDescription::destroy() {
    allocator_.destroy(*this);
}

} // namespace roc
} // namespace sdp
