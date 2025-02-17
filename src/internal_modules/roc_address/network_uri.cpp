/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/network_uri.h"
#include "roc_address/pct.h"
#include "roc_address/protocol_map.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

namespace {

bool safe_strcmp(const char* a, const char* b) {
    if (a == NULL || b == NULL) {
        return a == b;
    }
    return strcmp(a, b) == 0;
}

} // namespace

NetworkUri::NetworkUri(core::IArena& arena)
    : invalid_parts_(0)
    , host_(arena)
    , path_(arena)
    , query_(arena) {
    clear(Subset_Full);
}

bool NetworkUri::is_equal(const NetworkUri& other) const {
    if (invalid_parts_ != 0 || other.invalid_parts_ != 0) {
        return false;
    }

    if (proto() != other.proto()) {
        return false;
    }

    if (!safe_strcmp(host(), other.host())) {
        return false;
    }

    if (port() != other.port()) {
        return false;
    }

    if (!safe_strcmp(path(), other.path())) {
        return false;
    }

    if (!safe_strcmp(encoded_query(), other.encoded_query())) {
        return false;
    }

    return true;
}

bool NetworkUri::assign(const NetworkUri& other) {
    clear(Subset_Full);

    invalidate(Subset_Full);

    if (!set_proto(other.proto())) {
        return false;
    }

    if (!set_host(other.host())) {
        return false;
    }

    if (!set_port(other.port())) {
        return false;
    }

    if (!set_path(other.path())) {
        return false;
    }

    if (!set_encoded_query(other.encoded_query())) {
        return false;
    }

    return true;
}

bool NetworkUri::verify(Subset subset) const {
    if (subset == Subset_Resource) {
        if ((invalid_parts_ & (PartPath | PartQuery)) != 0) {
            roc_log(LogError, "invalid endpoint uri: contains invalid parts");
            return false;
        }

        return true;
    }

    if (invalid_parts_ != 0) {
        roc_log(LogError, "invalid endpoint uri: contains invalid parts");
        return false;
    }

    if (service_[0] == '\0') {
        roc_log(LogError, "invalid endpoint uri: unknown service");
        return false;
    }

    const ProtocolAttrs* proto_attrs = ProtocolMap::instance().find_by_id(proto_);
    if (!proto_attrs) {
        roc_log(LogError, "invalid endpoint uri: unknown protocol");
        return false;
    }

    if (port_ < 0 && proto_attrs->default_port < 0) {
        roc_log(LogError,
                "invalid endpoint uri:"
                " protocol '%s' requires a port to be specified explicitly,"
                " but it is omitted in the uri",
                proto_to_str(proto_));
        return false;
    }

    if (!proto_attrs->path_supported) {
        if (!path_.is_empty() || !query_.is_empty()) {
            roc_log(LogError,
                    "invalid endpoint uri:"
                    " protocol '%s' forbids using a path and query,"
                    " but they are present in the uri",
                    proto_to_str(proto_));
            return false;
        }
    }

    return true;
}

void NetworkUri::clear(Subset subset) {
    if (subset == Subset_Full) {
        invalid_parts_ |= PartProto;
        proto_ = Proto_None;

        invalid_parts_ |= PartHost;
        host_.clear();

        invalid_parts_ |= PartPort;
        port_ = -1;
        service_[0] = '\0';
    }

    invalid_parts_ &= ~PartPath;
    path_.clear();

    invalid_parts_ &= ~PartQuery;
    query_.clear();
}

void NetworkUri::invalidate(Subset subset) {
    if (subset == Subset_Full) {
        invalid_parts_ |= (PartProto | PartHost | PartPort);
    }
    invalid_parts_ |= (PartPath | PartQuery);
}

bool NetworkUri::set_proto(Protocol proto) {
    if (ProtocolMap::instance().find_by_id(proto) == NULL) {
        set_invalid_(PartProto);
        return false;
    }

    proto_ = proto;

    if (port_ == -1) {
        if (set_service_from_proto_(proto)) {
            set_valid_(PartPort);
        } else {
            set_invalid_(PartPort);
        }
    }

    set_valid_(PartProto);
    return true;
}

Protocol NetworkUri::proto() const {
    if (!part_is_valid_(PartProto)) {
        return Proto_None;
    }
    return proto_;
}

bool NetworkUri::get_proto(Protocol& proto) const {
    if (!part_is_valid_(PartProto)) {
        return false;
    }

    proto = proto_;
    return true;
}

bool NetworkUri::format_proto(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartProto)) {
        return false;
    }

    const ProtocolAttrs* attrs = ProtocolMap::instance().find_by_id(proto_);
    if (!attrs) {
        return false;
    }

    dst.append_str(attrs->scheme_name);
    return true;
}

bool NetworkUri::set_host(const char* str) {
    if (!str) {
        set_invalid_(PartHost);
        return false;
    }

    return set_host(str, strlen(str));
}

bool NetworkUri::set_host(const char* str, size_t str_len) {
    if (!str) {
        set_invalid_(PartHost);
        return false;
    }

    if (!host_.assign(str, str + str_len) || host_.is_empty()) {
        set_invalid_(PartHost);
        return false;
    }

    set_valid_(PartHost);
    return true;
}

const char* NetworkUri::host() const {
    if (!part_is_valid_(PartHost)) {
        return "";
    }
    return host_.c_str();
}

bool NetworkUri::format_host(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartHost)) {
        return false;
    }
    dst.append_str(host_.c_str());
    return true;
}

bool NetworkUri::set_port(int port) {
    if (port == -1) {
        port_ = -1;

        if (part_is_valid_(PartProto)) {
            if (set_service_from_proto_(proto_)) {
                set_valid_(PartPort);
            } else {
                set_invalid_(PartPort);
            }
        } else {
            set_invalid_(PartPort);
        }

        return true;
    }

    if (port < 0 || port > 65535) {
        set_invalid_(PartPort);
        return false;
    }

    port_ = port;

    set_service_from_port_(port);
    set_valid_(PartPort);

    return true;
}

int NetworkUri::port() const {
    if (!part_is_valid_(PartPort)) {
        return -1;
    }
    return port_;
}

bool NetworkUri::get_port(int& port) const {
    if (!part_is_valid_(PartPort) || port_ == -1) {
        return false;
    }

    port = port_;
    return true;
}

const char* NetworkUri::service() const {
    if (service_[0]) {
        return service_;
    }
    return NULL;
}

bool NetworkUri::set_path(const char* str) {
    if (!str) {
        path_.clear();
        set_valid_(PartPath);
        return true;
    }

    return set_path(str, strlen(str));
}

bool NetworkUri::set_path(const char* str, size_t str_len) {
    if (!str || str_len < 1) {
        path_.clear();
        set_valid_(PartPath);
        return true;
    }

    if (!path_.assign(str, str + str_len)) {
        set_invalid_(PartPath);
        return false;
    }

    set_valid_(PartPath);
    return true;
}

bool NetworkUri::set_encoded_path(const char* str) {
    if (!str) {
        path_.clear();
        set_valid_(PartPath);
        return true;
    }

    return set_encoded_path(str, strlen(str));
}

bool NetworkUri::set_encoded_path(const char* str, size_t str_len) {
    if (!str || str_len < 1) {
        path_.clear();
        set_valid_(PartPath);
        return true;
    }

    if (!path_.grow(str_len)) {
        set_invalid_(PartPath);
        return false;
    }

    core::StringBuilder b(path_);

    if (!pct_decode(b, str, str_len)) {
        set_invalid_(PartPath);
        return false;
    }

    if (!b.is_ok()) {
        set_invalid_(PartPath);
        return false;
    }

    set_valid_(PartPath);
    return true;
}

const char* NetworkUri::path() const {
    if (!part_is_valid_(PartPath) || path_.is_empty()) {
        return NULL;
    }
    return path_.c_str();
}

bool NetworkUri::format_encoded_path(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartPath) || path_.is_empty()) {
        return false;
    }
    return pct_encode(dst, path_.c_str(), path_.len(), PctNonPath);
}

bool NetworkUri::set_encoded_query(const char* str) {
    if (!str) {
        query_.clear();
        set_valid_(PartQuery);
        return true;
    }

    return set_encoded_query(str, strlen(str));
}

bool NetworkUri::set_encoded_query(const char* str, size_t str_len) {
    if (!str || str_len < 1) {
        query_.clear();
        set_valid_(PartQuery);
        return true;
    }

    if (!query_.assign(str, str + str_len)) {
        set_invalid_(PartQuery);
        return false;
    }

    set_valid_(PartQuery);
    return true;
}

const char* NetworkUri::encoded_query() const {
    if (!part_is_valid_(PartQuery) || query_.is_empty()) {
        return NULL;
    }
    return query_.c_str();
}

bool NetworkUri::format_encoded_query(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartQuery) || query_.is_empty()) {
        return false;
    }
    dst.append_str(query_.c_str());
    return true;
}

void NetworkUri::set_service_from_port_(int port) {
    core::StringBuilder b(service_, sizeof(service_));

    if (!b.append_uint((uint64_t)port, 10)) {
        roc_panic("endpoint uri: can't format port to string");
    }
}

bool NetworkUri::set_service_from_proto_(Protocol proto) {
    const ProtocolAttrs* attrs = ProtocolMap::instance().find_by_id(proto);
    if (!attrs) {
        return false;
    }

    if (attrs->default_port <= 0) {
        return false;
    }

    set_service_from_port_(attrs->default_port);
    return true;
}

bool NetworkUri::part_is_valid_(Part part) const {
    return (invalid_parts_ & part) == 0;
}

void NetworkUri::set_valid_(Part part) {
    invalid_parts_ &= ~part;
}

void NetworkUri::set_invalid_(Part part) {
    invalid_parts_ |= part;
}

} // namespace address
} // namespace roc
