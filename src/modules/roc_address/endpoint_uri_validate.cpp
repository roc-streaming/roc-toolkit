/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_type_to_str.h"
#include "roc_address/endpoint_uri.h"
#include "roc_address/proto.h"

namespace roc {
namespace address {

namespace {

bool check_protocol_type(EndpointType type, EndpointProtocol proto) {
    switch (type) {
    case EndType_Session:
        switch ((int)proto) {
        case EndProto_RTSP:
            return true;
        default:
            break;
        }
        return false;

    case EndType_AudioSource:
        switch ((int)proto) {
        case EndProto_RTP:
        case EndProto_RTP_RS8M_Source:
        case EndProto_RTP_LDPC_Source:
            return true;
        default:
            break;
        }
        return false;

    case EndType_AudioRepair:
        switch ((int)proto) {
        case EndProto_RS8M_Repair:
        case EndProto_LDPC_Repair:
            return true;
        default:
            break;
        }
        return false;
    }

    return false;
}

} // namespace

bool validate_endpoint_uri(EndpointType type, const EndpointURI& uri) {
    if (!uri.is_valid()) {
        roc_log(LogError, "invalid endpoint uri: missing scheme or host");
        return false;
    }

    if (!check_protocol_type(type, uri.proto())) {
        roc_log(LogError,
                "invalid endpoint uri:"
                " endpoint protocol '%s' can't be used for %s endpoints",
                proto_to_str(uri.proto()), endpoint_type_to_str(type));
        return false;
    }

    if (uri.port() < 0 && proto_default_port(uri.proto()) < 0) {
        roc_log(LogError,
                "invalid endpoint uri:"
                " endpoint protocol '%s' requires a port to be specified explicitly,"
                " but it is omitted in the uri",
                proto_to_str(uri.proto()));
        return false;
    }

    if (!proto_supports_path(uri.proto())) {
        if (uri.path() || uri.encoded_query() || uri.encoded_fragment()) {
            roc_log(LogError,
                    "invalid endpoint uri:"
                    " endpoint protocol '%s' forbids using a path, query, and fragment,"
                    " but they are present in the uri",
                    proto_to_str(uri.proto()));
            return false;
        }
    }

    return true;
}

} // namespace address
} // namespace roc
