/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/endpoint_helpers.h"
#include "roc_address/protocol_map.h"
#include "roc_core/log.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/fec_scheme_to_str.h"

namespace roc {
namespace pipeline {

bool validate_endpoint(address::EndpointType type, address::EndpointProtocol proto) {
    const address::ProtocolAttrs* proto_attrs =
        address::ProtocolMap::instance().find_proto(proto);

    if (!proto_attrs) {
        roc_log(LogError, "bad endpoints configuration: unknown protocol");
        return false;
    }

    if (proto_attrs->type != type) {
        roc_log(
            LogError,
            "bad endpoints configuration: %s protocol is not suitable for %s endpoint",
            address::endpoint_proto_to_str(proto), address::endpoint_type_to_str(type));
        return false;
    }

    if (proto_attrs->fec_scheme != packet::FEC_None
        && !fec::CodecMap::instance().is_supported(proto_attrs->fec_scheme)) {
        roc_log(LogError,
                "bad endpoints configuration:"
                " %s endpoint protocol '%s' implies fec scheme '%s',"
                " but it is disabled at compile time",
                address::endpoint_type_to_str(type),
                address::endpoint_proto_to_str(proto),
                packet::fec_scheme_to_str(proto_attrs->fec_scheme));
        return false;
    }

    return true;
}

bool validate_endpoint_and_pipeline_consistency(packet::FecScheme pipeline_fec_scheme,
                                                address::EndpointType type,
                                                address::EndpointProtocol proto) {
    const address::ProtocolAttrs* proto_attrs =
        address::ProtocolMap::instance().find_proto(proto);

    if (!proto_attrs) {
        roc_log(LogError, "bad endpoints configuration: unknown protocol");
        return false;
    }

    if (type == address::EndType_AudioRepair && proto != address::EndProto_None
        && pipeline_fec_scheme == packet::FEC_None) {
        roc_log(LogError,
                "bad endpoints configuration:"
                " repair endpoint is provided,"
                " but pipeline is not configured to use any fec scheme");
        return false;
    }

    if (proto_attrs->fec_scheme != pipeline_fec_scheme) {
        roc_log(LogError,
                "bad endpoints configuration:"
                " %s endpoint protocol '%s' implies fec scheme '%s',"
                " but pipeline is configured to use fec scheme '%s'",
                address::endpoint_type_to_str(type),
                address::endpoint_proto_to_str(proto),
                packet::fec_scheme_to_str(proto_attrs->fec_scheme),
                packet::fec_scheme_to_str(pipeline_fec_scheme));
        return false;
    }

    return true;
}

// note: many of the checks below are redundant, but they help
// to provide meaninful error messages
bool validate_endpoint_pair_consistency(address::EndpointProtocol source_proto,
                                        address::EndpointProtocol repair_proto) {
    // source endpoint is missing
    if (source_proto == address::EndProto_None) {
        roc_log(LogError, "bad endpoints configuration: no source endpoint provided");
        return false;
    }

    const address::ProtocolAttrs* source_attrs =
        address::ProtocolMap::instance().find_proto(source_proto);

    if (!source_attrs) {
        roc_log(LogError, "bad endpoints configuration: unknown source protocol");
        return false;
    }

    // repair endpoint is needed but missing
    if (source_attrs->fec_scheme != packet::FEC_None
        && repair_proto == address::EndProto_None) {
        roc_log(
            LogError,
            "bad endpoints configuration:"
            " source endpoint protocol '%s' implies fec scheme '%s' and two endpoints,"
            " but repair endpoint is not provided",
            address::endpoint_proto_to_str(source_proto),
            packet::fec_scheme_to_str(source_attrs->fec_scheme));
        return false;
    }

    // repair endpoint is not needed but present
    if (source_attrs->fec_scheme == packet::FEC_None
        && repair_proto != address::EndProto_None) {
        roc_log(LogError,
                "bad endpoints configuration:"
                " source endpoint protocol '%s' implies no fec scheme and one endpoint,"
                " but repair endpoint is provided",
                address::endpoint_proto_to_str(source_proto));
        return false;
    }

    if (repair_proto != address::EndProto_None) {
        const address::ProtocolAttrs* repair_attrs =
            address::ProtocolMap::instance().find_proto(repair_proto);

        if (!repair_attrs) {
            roc_log(LogError, "bad endpoints configuration: unknown repair protocol");
            return false;
        }

        // source and repair endpoints are inconsistent
        if (source_attrs->fec_scheme != repair_attrs->fec_scheme) {
            roc_log(LogError,
                    "bad endpoints configuration:"
                    " source endpoint protocol '%s' implies fec scheme '%s',"
                    " but repair endpoint protocolo '%s' implies fec scheme '%s'",
                    address::endpoint_proto_to_str(source_proto),
                    packet::fec_scheme_to_str(source_attrs->fec_scheme),
                    address::endpoint_proto_to_str(repair_proto),
                    packet::fec_scheme_to_str(repair_attrs->fec_scheme));
            return false;
        }
    }

    return true;
}

} // namespace pipeline
} // namespace roc
