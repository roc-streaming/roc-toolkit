/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_address/endpoint_uri.h"
#include "roc_address/protocol_map.h"

namespace roc {
namespace address {

bool validate_endpoint_uri(const EndpointURI& uri) {
    if (!uri.is_valid()) {
        roc_log(LogError, "invalid endpoint uri: missing scheme or host");
        return false;
    }

    const ProtocolAttrs* proto_attrs = ProtocolMap::instance().find_proto(uri.proto());
    if (!proto_attrs) {
        roc_log(LogError, "invalid endpoint uri: unknown protocol");
        return false;
    }

    if (uri.port() < 0 && proto_attrs->default_port < 0) {
        roc_log(LogError,
                "invalid endpoint uri:"
                " endpoint protocol '%s' requires a port to be specified explicitly,"
                " but it is omitted in the uri",
                endpoint_proto_to_str(uri.proto()));
        return false;
    }

    if (!proto_attrs->path_supported) {
        if (uri.path() || uri.encoded_query() || uri.encoded_fragment()) {
            roc_log(LogError,
                    "invalid endpoint uri:"
                    " endpoint protocol '%s' forbids using a path, query, and fragment,"
                    " but they are present in the uri",
                    endpoint_proto_to_str(uri.proto()));
            return false;
        }
    }

    return true;
}

} // namespace address
} // namespace roc
