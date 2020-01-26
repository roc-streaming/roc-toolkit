/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri.h"
#include "roc_address/pct.h"
#include "roc_address/protocol_map.h"
#include "roc_core/string_utils.h"

namespace roc {
namespace address {

EndpointURI::EndpointURI(core::IAllocator& allocator)
    : host_(allocator)
    , path_(allocator)
    , query_(allocator)
    , frag_(allocator) {
    clear();
}

bool EndpointURI::is_valid() const {
    return proto_ != EndProto_None && host_.size() != 0;
}

void EndpointURI::clear() {
    proto_ = EndProto_None;
    host_.resize(0);
    port_ = -1;
    service_[0] = '\0';
    path_.resize(0);
    query_.resize(0);
    frag_.resize(0);
}

EndpointProtocol EndpointURI::proto() const {
    if (!is_valid()) {
        return EndProto_None;
    }
    return proto_;
}

void EndpointURI::set_proto(EndpointProtocol proto) {
    proto_ = proto;

    if (port_ == -1) {
        set_service_from_proto_(proto);
    }
}

const char* EndpointURI::host() const {
    if (!is_valid()) {
        return "";
    }
    return &host_[0];
}

bool EndpointURI::set_host(const char* str) {
    const size_t str_len = strlen(str);

    if (str_len < 1) {
        host_.resize(0);
        return true;
    }

    if (!host_.resize(str_len + 1)) {
        return false;
    }

    if (!core::copy_str(&host_[0], host_.size(), str, str + str_len)) {
        return false;
    }

    return true;
}

bool EndpointURI::set_encoded_host(const char* str, size_t str_len) {
    if (str_len < 1) {
        return false;
    }

    const size_t buf_size = str_len + 1;

    if (!host_.resize(buf_size)) {
        return false;
    }

    if (pct_decode(&host_[0], buf_size, str, str_len) == -1) {
        return false;
    }

    return true;
}

bool EndpointURI::get_encoded_host(char* str, size_t str_len) const {
    if (!is_valid()) {
        return false;
    }
    return pct_encode(str, str_len, &host_[0], strlen(&host_[0]), PctNonHost) != -1;
}

int EndpointURI::port() const {
    return port_;
}

bool EndpointURI::set_port(int port) {
    if (port < 0 || port > 65535) {
        return false;
    }

    port_ = port;

    set_service_from_port_(port);

    return true;
}

const char* EndpointURI::service() const {
    if (service_[0]) {
        return service_;
    }
    return NULL;
}

void EndpointURI::set_service_from_port_(int port) {
    service_[0] = '\0';
    if (!core::append_uint(service_, sizeof(service_), (uint64_t)port, 10)) {
        roc_panic("endpoint uri: can't format port to string");
    }
}

void EndpointURI::set_service_from_proto_(EndpointProtocol proto) {
    const ProtocolAttrs* attrs = ProtocolMap::instance().find_proto(proto);
    if (!attrs) {
        return;
    }

    if (attrs->default_port <= 0) {
        return;
    }

    set_service_from_port_(attrs->default_port);
}

const char* EndpointURI::path() const {
    if (!is_valid() || path_.size() == 0) {
        return NULL;
    }
    return &path_[0];
}

bool EndpointURI::set_encoded_path(const char* str, size_t str_len) {
    if (str_len < 1) {
        path_.resize(0);
        return true;
    }

    const size_t buf_size = str_len + 1;

    if (!path_.resize(buf_size)) {
        return false;
    }

    if (pct_decode(&path_[0], buf_size, str, str_len) == -1) {
        return false;
    }

    return true;
}

bool EndpointURI::get_encoded_path(char* str, size_t str_len) const {
    if (!is_valid() || path_.size() == 0) {
        return false;
    }
    return pct_encode(str, str_len, &path_[0], strlen(&path_[0]), PctNonPath) != -1;
}

const char* EndpointURI::encoded_query() const {
    if (!is_valid() || query_.size() == 0) {
        return NULL;
    }
    return &query_[0];
}

bool EndpointURI::set_encoded_query(const char* str, size_t str_len) {
    if (str_len < 1) {
        query_.resize(0);
        return true;
    }

    const size_t buf_size = str_len + 1;

    if (!query_.resize(buf_size)) {
        return false;
    }

    if (!core::copy_str(&query_[0], buf_size, str, str + str_len)) {
        return false;
    }

    return true;
}

const char* EndpointURI::encoded_fragment() const {
    if (!is_valid() || frag_.size() == 0) {
        return NULL;
    }
    return &frag_[0];
}

bool EndpointURI::set_encoded_fragment(const char* str, size_t str_len) {
    if (str_len < 1) {
        frag_.resize(0);
        return true;
    }

    const size_t buf_size = str_len + 1;

    if (!frag_.resize(buf_size)) {
        return false;
    }

    if (!core::copy_str(&frag_[0], buf_size, str, str + str_len)) {
        return false;
    }

    return true;
}

} // namespace address
} // namespace roc
