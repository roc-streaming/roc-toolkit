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

namespace roc {
namespace address {

EndpointURI::EndpointURI(core::IAllocator& allocator)
    : invalid_parts_(0)
    , host_(allocator)
    , path_(allocator)
    , query_(allocator)
    , frag_(allocator) {
    clear(Subset_Full);
}

bool EndpointURI::check(Subset subset) const {
    if (subset == Subset_Resource) {
        if ((invalid_parts_ & (PartPath | PartQuery | PartFrag)) != 0) {
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

    const ProtocolAttrs* proto_attrs = ProtocolMap::instance().find_proto(proto_);
    if (!proto_attrs) {
        roc_log(LogError, "invalid endpoint uri: unknown protocol");
        return false;
    }

    if (port_ < 0 && proto_attrs->default_port < 0) {
        roc_log(LogError,
                "invalid endpoint uri:"
                " endpoint protocol '%s' requires a port to be specified explicitly,"
                " but it is omitted in the uri",
                endpoint_proto_to_str(proto_));
        return false;
    }

    if (!proto_attrs->path_supported) {
        if (!path_.is_empty() || !query_.is_empty() || !frag_.is_empty()) {
            roc_log(LogError,
                    "invalid endpoint uri:"
                    " endpoint protocol '%s' forbids using a path, query, and fragment,"
                    " but they are present in the uri",
                    endpoint_proto_to_str(proto_));
            return false;
        }
    }

    return true;
}

void EndpointURI::clear(Subset subset) {
    if (subset == Subset_Full) {
        invalid_parts_ |= PartProto;
        proto_ = EndProto_None;

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

    invalid_parts_ &= ~PartFrag;
    frag_.clear();
}

void EndpointURI::invalidate(Subset subset) {
    if (subset == Subset_Full) {
        invalid_parts_ |= (PartProto | PartHost | PartPort);
    }
    invalid_parts_ |= (PartPath | PartQuery | PartFrag);
}

bool EndpointURI::part_is_valid_(Part part) const {
    return (invalid_parts_ & part) == 0;
}

void EndpointURI::set_valid_(Part part) {
    invalid_parts_ &= ~part;
}

void EndpointURI::set_invalid_(Part part) {
    invalid_parts_ |= part;
}

EndpointProtocol EndpointURI::proto() const {
    if (!part_is_valid_(PartProto)) {
        return EndProto_None;
    }
    return proto_;
}

bool EndpointURI::set_proto(EndpointProtocol proto) {
    if (ProtocolMap::instance().find_proto(proto) == NULL) {
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

bool EndpointURI::get_proto(EndpointProtocol& proto) const {
    if (!part_is_valid_(PartProto)) {
        return false;
    }

    proto = proto_;
    return true;
}

const char* EndpointURI::host() const {
    if (!part_is_valid_(PartHost)) {
        return "";
    }
    return host_.c_str();
}

bool EndpointURI::set_host(const char* str) {
    if (!str) {
        set_invalid_(PartHost);
        return false;
    }

    if (!host_.set_str(str) || host_.is_empty()) {
        set_invalid_(PartHost);
        return false;
    }

    set_valid_(PartHost);
    return true;
}

bool EndpointURI::set_host(const char* str, size_t str_len) {
    if (!str) {
        set_invalid_(PartHost);
        return false;
    }

    if (!host_.set_buf(str, str_len) || host_.is_empty()) {
        set_invalid_(PartHost);
        return false;
    }

    set_valid_(PartHost);
    return true;
}

bool EndpointURI::format_host(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartHost)) {
        return false;
    }
    dst.append_str(host_.c_str());
    return true;
}

int EndpointURI::port() const {
    if (!part_is_valid_(PartPort)) {
        return -1;
    }
    return port_;
}

bool EndpointURI::set_port(int port) {
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

bool EndpointURI::get_port(int& port) const {
    if (!part_is_valid_(PartPort) || port_ == -1) {
        return false;
    }

    port = port_;
    return true;
}

const char* EndpointURI::service() const {
    if (service_[0]) {
        return service_;
    }
    return NULL;
}

void EndpointURI::set_service_from_port_(int port) {
    core::StringBuilder b(service_, sizeof(service_));

    if (!b.append_uint((uint64_t)port, 10)) {
        roc_panic("endpoint uri: can't format port to string");
    }
}

bool EndpointURI::set_service_from_proto_(EndpointProtocol proto) {
    const ProtocolAttrs* attrs = ProtocolMap::instance().find_proto(proto);
    if (!attrs) {
        return false;
    }

    if (attrs->default_port <= 0) {
        return false;
    }

    set_service_from_port_(attrs->default_port);
    return true;
}

const char* EndpointURI::path() const {
    if (!part_is_valid_(PartPath) || path_.is_empty()) {
        return NULL;
    }
    return path_.c_str();
}

bool EndpointURI::set_encoded_path(const char* str, size_t str_len) {
    if (!str || str_len < 1) {
        path_.clear();
        set_valid_(PartPath);
        return true;
    }

    if (!path_.grow(str_len + 1)) {
        set_invalid_(PartPath);
        return false;
    }

    core::StringBuilder b(path_.raw_buf());

    if (!pct_decode(b, str, str_len)) {
        set_invalid_(PartPath);
        return false;
    }

    if (!b.ok()) {
        set_invalid_(PartPath);
        return false;
    }

    set_valid_(PartPath);
    return true;
}

bool EndpointURI::format_encoded_path(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartPath) || path_.is_empty()) {
        return false;
    }
    return pct_encode(dst, path_.c_str(), path_.len(), PctNonPath);
}

const char* EndpointURI::encoded_query() const {
    if (!part_is_valid_(PartQuery) || query_.is_empty()) {
        return NULL;
    }
    return query_.c_str();
}

bool EndpointURI::set_encoded_query(const char* str, size_t str_len) {
    if (!str || str_len < 1) {
        query_.clear();
        set_valid_(PartQuery);
        return true;
    }

    if (!query_.set_buf(str, str_len)) {
        set_invalid_(PartQuery);
        return false;
    }

    set_valid_(PartQuery);
    return true;
}

bool EndpointURI::format_encoded_query(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartQuery) || query_.is_empty()) {
        return false;
    }
    dst.append_str(query_.c_str());
    return true;
}

const char* EndpointURI::encoded_fragment() const {
    if (!part_is_valid_(PartFrag) || frag_.is_empty()) {
        return NULL;
    }
    return frag_.c_str();
}

bool EndpointURI::set_encoded_fragment(const char* str, size_t str_len) {
    if (!str || str_len < 1) {
        frag_.clear();
        set_valid_(PartFrag);
        return true;
    }

    if (!frag_.set_buf(str, str_len)) {
        set_invalid_(PartFrag);
        return false;
    }

    set_valid_(PartFrag);
    return true;
}

bool EndpointURI::format_encoded_fragment(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartFrag) || frag_.is_empty()) {
        return false;
    }
    dst.append_str(frag_.c_str());
    return true;
}

} // namespace address
} // namespace roc
