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
    : non_empty_fields_(0)
    , broken_fields_(0)
    , host_(arena)
    , path_(arena)
    , query_(arena) {
    clear_fields(FieldsAll);
}

bool NetworkUri::operator==(const NetworkUri& other) const {
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

bool NetworkUri::operator!=(const NetworkUri& other) const {
    return !(*this == other);
}

bool NetworkUri::is_valid() const {
    if (field_state_(FieldProto) != NotEmpty) {
        roc_log(LogError, "invalid endpoint uri: missing protocol");
        return false;
    }

    if (field_state_(FieldHost) != NotEmpty) {
        roc_log(LogError, "invalid endpoint uri: missing host");
        return false;
    }

    const ProtocolAttrs* proto_attrs = ProtocolMap::instance().find_by_id(proto_);
    if (!proto_attrs) {
        roc_log(LogError, "invalid endpoint uri: unknown protocol");
        return false;
    }

    if (proto_attrs->default_port > 0) {
        if (field_state_(FieldPort) == Broken) {
            roc_log(LogError, "invalid endpoint uri: invalid port");
            return false;
        }
    } else {
        if (field_state_(FieldPort) != NotEmpty) {
            roc_log(LogError,
                    "invalid endpoint uri:"
                    " protocol '%s' requires a port to be specified explicitly,"
                    " but it is omitted in the uri",
                    proto_to_str(proto_));
            return false;
        }
    }

    if (field_state_(FieldPath) == Broken) {
        roc_log(LogError, "invalid endpoint uri: invalid path");
        return false;
    }
    if (field_state_(FieldQuery) == Broken) {
        roc_log(LogError, "invalid endpoint uri: invalid query");
        return false;
    }
    if (!proto_attrs->path_supported) {
        if (field_state_(FieldPath) != Empty || field_state_(FieldQuery) != Empty) {
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

bool NetworkUri::has_fields(int fields_mask) const {
    for (size_t n = 0; n < sizeof(fields_mask) * 8; n++) {
        if ((fields_mask & (int)n) && field_state_((Field)n) != NotEmpty) {
            return false;
        }
    }

    return true;
}

void NetworkUri::clear_fields(int fields_mask) {
    if (fields_mask & FieldProto) {
        set_field_state_(FieldProto, Empty);
        proto_ = Proto_None;
    }

    if (fields_mask & FieldHost) {
        set_field_state_(FieldHost, Empty);
        host_.clear();
    }

    if (fields_mask & FieldPort) {
        set_field_state_(FieldPort, Empty);
        port_ = -1;
    }

    if (fields_mask & FieldPath) {
        set_field_state_(FieldPath, Empty);
        path_.clear();
    }

    if (fields_mask & FieldQuery) {
        set_field_state_(FieldQuery, Empty);
        query_.clear();
    }
}

void NetworkUri::invalidate_fields(int fields_mask) {
    if (fields_mask & FieldProto) {
        set_field_state_(FieldProto, Broken);
        proto_ = Proto_None;
    }

    if (fields_mask & FieldHost) {
        set_field_state_(FieldHost, Broken);
        host_.clear();
    }

    if (fields_mask & FieldPort) {
        set_field_state_(FieldPort, Broken);
        port_ = -1;
    }

    if (fields_mask & FieldPath) {
        set_field_state_(FieldPath, Broken);
        path_.clear();
    }

    if (fields_mask & FieldQuery) {
        set_field_state_(FieldQuery, Broken);
        query_.clear();
    }
}

bool NetworkUri::assign(const NetworkUri& other) {
    clear_fields(FieldsAll);

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

bool NetworkUri::set_proto(Protocol proto) {
    if (ProtocolMap::instance().find_by_id(proto) == NULL) {
        proto_ = Proto_None;
        set_field_state_(FieldProto, Broken);
        return false;
    }

    proto_ = proto;
    set_field_state_(FieldProto, NotEmpty);

    return true;
}

Protocol NetworkUri::proto() const {
    if (field_state_(FieldProto) != NotEmpty) {
        return Proto_None;
    }

    return proto_;
}

bool NetworkUri::get_proto(Protocol& proto) const {
    if (field_state_(FieldProto) != NotEmpty) {
        return false;
    }

    proto = proto_;
    return true;
}

bool NetworkUri::format_proto(core::StringBuilder& dst) const {
    if (field_state_(FieldProto) != NotEmpty) {
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
        host_.clear();
        set_field_state_(FieldHost, Broken);
        return false;
    }

    return set_host(str, strlen(str));
}

bool NetworkUri::set_host(const char* str, size_t str_len) {
    if (!str || str_len == 0) {
        host_.clear();
        set_field_state_(FieldHost, Broken);
        return false;
    }

    if (!host_.assign(str, str + str_len) || host_.is_empty()) {
        host_.clear();
        set_field_state_(FieldHost, Broken);
        return false;
    }

    set_field_state_(FieldHost, NotEmpty);
    return true;
}

const char* NetworkUri::host() const {
    if (field_state_(FieldHost) != NotEmpty) {
        return "";
    }
    return host_.c_str();
}

bool NetworkUri::format_host(core::StringBuilder& dst) const {
    if (field_state_(FieldHost) != NotEmpty) {
        return false;
    }

    dst.append_str(host_.c_str());
    return true;
}

bool NetworkUri::set_port(int port) {
    if (port == DefautPort) {
        port_ = DefautPort;
        set_field_state_(FieldPort, Empty);
        return true;
    }

    if (port < 0 || port > 65535) {
        port_ = DefautPort;
        set_field_state_(FieldPort, Broken);
        return false;
    }

    port_ = port;
    set_field_state_(FieldPort, NotEmpty);
    return true;
}

int NetworkUri::port() const {
    if (field_state_(FieldPort) != NotEmpty) {
        return DefautPort;
    }

    return port_;
}

bool NetworkUri::get_port(int& port) const {
    if (field_state_(FieldPort) != NotEmpty) {
        return false;
    }

    port = port_;
    return true;
}

int NetworkUri::port_or_default() const {
    if (field_state_(FieldPort) == NotEmpty) {
        return port_;
    }

    if (field_state_(FieldProto) == NotEmpty) {
        const ProtocolAttrs* attrs = ProtocolMap::instance().find_by_id(proto_);
        if (attrs) {
            if (attrs->default_port > 0) {
                return attrs->default_port;
            }
        }
    }

    return DefautPort;
}

bool NetworkUri::set_path(const char* str) {
    if (!str) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    return set_path(str, strlen(str));
}

bool NetworkUri::set_path(const char* str, size_t str_len) {
    if (!str) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    if (!path_.assign(str, str + str_len)) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    set_field_state_(FieldPath, str_len > 0 ? NotEmpty : Empty);
    return true;
}

bool NetworkUri::set_encoded_path(const char* str) {
    if (!str) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    return set_encoded_path(str, strlen(str));
}

bool NetworkUri::set_encoded_path(const char* str, size_t str_len) {
    if (!str) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    if (!path_.grow(str_len)) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    core::StringBuilder b(path_);

    if (!pct_decode(b, str, str_len)) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    if (!b.is_ok()) {
        path_.clear();
        set_field_state_(FieldPath, Broken);
        return false;
    }

    set_field_state_(FieldPath, str_len > 0 ? NotEmpty : Empty);
    return true;
}

const char* NetworkUri::path() const {
    if (field_state_(FieldPath) != NotEmpty) {
        return NULL;
    }
    return path_.c_str();
}

bool NetworkUri::format_encoded_path(core::StringBuilder& dst) const {
    if (field_state_(FieldPath) != NotEmpty) {
        return false;
    }

    return pct_encode(dst, path_.c_str(), path_.len(), PctNonPath);
}

bool NetworkUri::set_encoded_query(const char* str) {
    if (!str) {
        query_.clear();
        set_field_state_(FieldQuery, Broken);
        return false;
    }

    return set_encoded_query(str, strlen(str));
}

bool NetworkUri::set_encoded_query(const char* str, size_t str_len) {
    if (!str) {
        query_.clear();
        set_field_state_(FieldQuery, Broken);
        return false;
    }

    if (!query_.assign(str, str + str_len)) {
        query_.clear();
        set_field_state_(FieldQuery, Broken);
        return false;
    }

    set_field_state_(FieldQuery, str_len > 0 ? NotEmpty : Empty);
    return true;
}

const char* NetworkUri::encoded_query() const {
    if (field_state_(FieldQuery) != NotEmpty) {
        return NULL;
    }

    return query_.c_str();
}

bool NetworkUri::format_encoded_query(core::StringBuilder& dst) const {
    if (field_state_(FieldQuery) != NotEmpty) {
        return false;
    }

    dst.append_str(query_.c_str());
    return true;
}

NetworkUri::FieldState NetworkUri::field_state_(Field field) const {
    if (broken_fields_ & field) {
        return Broken;
    }

    if (non_empty_fields_ & field) {
        return NotEmpty;
    }

    return Empty;
}

void NetworkUri::set_field_state_(Field field, FieldState state) {
    if (state == Broken) {
        broken_fields_ |= field;
    } else {
        broken_fields_ &= ~field;
    }

    if (state == NotEmpty) {
        non_empty_fields_ |= field;
    } else {
        non_empty_fields_ &= ~field;
    }
}

} // namespace address
} // namespace roc
