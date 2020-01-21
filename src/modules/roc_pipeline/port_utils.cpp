/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/port_utils.h"
#include "roc_core/log.h"
#include "roc_packet/fec_scheme_to_str.h"
#include "roc_pipeline/port_to_str.h"

namespace roc {
namespace pipeline {

packet::FecScheme port_fec_scheme(PortProtocol proto) {
    switch (proto) {
    case Proto_None:
        return packet::FEC_None;

    case Proto_RTP:
        return packet::FEC_None;

    case Proto_RTP_RSm8_Source:
        return packet::FEC_ReedSolomon_M8;

    case Proto_RSm8_Repair:
        return packet::FEC_ReedSolomon_M8;

    case Proto_RTP_LDPC_Source:
        return packet::FEC_LDPC_Staircase;

    case Proto_LDPC_Repair:
        return packet::FEC_LDPC_Staircase;
    }

    return packet::FEC_None;
}

bool validate_port(packet::FecScheme fec_scheme,
                   PortProtocol port_protocol,
                   PortType port_type) {
    const packet::FecScheme port_scheme = port_fec_scheme(port_protocol);

    if (port_type == Port_AudioRepair && port_protocol != Proto_None
        && fec_scheme == packet::FEC_None) {
        roc_log(
            LogError,
            "bad ports configuration:"
            " repair port is provided, but pipeline is configured to use no fec scheme");
        return false;
    }

    if (port_scheme != fec_scheme) {
        roc_log(LogError,
                "bad ports configuration:"
                " %s port protocol '%s' implies fec scheme '%s',"
                " but pipeline is configured to use fec scheme '%s'",
                port_type_to_str(port_type), port_proto_to_str(port_protocol),
                packet::fec_scheme_to_str(port_scheme),
                packet::fec_scheme_to_str(fec_scheme));
        return false;
    }

    return true;
}

bool validate_ports(packet::FecScheme fec_scheme,
                    PortProtocol source_port,
                    PortProtocol repair_port) {
    const packet::FecScheme source_port_scheme = port_fec_scheme(source_port);
    const packet::FecScheme repair_port_scheme = port_fec_scheme(repair_port);

    // source port is missing
    if (source_port == Proto_None) {
        roc_log(LogError, "bad ports configuration: no source port provided");
        return false;
    }

    // repair port is needed but missing
    if (source_port_scheme != packet::FEC_None && repair_port == Proto_None) {
        roc_log(LogError,
                "bad ports configuration:"
                " source port protocol '%s' implies fec scheme '%s' and two ports,"
                " but repair port is not provided",
                port_proto_to_str(source_port),
                packet::fec_scheme_to_str(source_port_scheme));
        return false;
    }

    // repair port is not needed but present
    if (source_port_scheme == packet::FEC_None && repair_port != Proto_None) {
        roc_log(LogError,
                "bad ports configuration:"
                " source port protocol '%s' implies no fec scheme and one port,"
                " but repair port is provided",
                port_proto_to_str(source_port));
        return false;
    }

    // source and repair ports are inconsistent
    if (source_port_scheme != repair_port_scheme) {
        roc_log(LogError,
                "bad ports configuration:"
                " source port protocol '%s' implies fec scheme '%s',"
                " but repair port protocol '%s' implies fec scheme '%s'",
                port_proto_to_str(source_port),
                packet::fec_scheme_to_str(source_port_scheme),
                port_proto_to_str(repair_port),
                packet::fec_scheme_to_str(repair_port_scheme));
        return false;
    }

    if (!validate_port(fec_scheme, source_port, Port_AudioSource)) {
        return false;
    }

    if (!validate_port(fec_scheme, repair_port, Port_AudioRepair)) {
        return false;
    }

    return true;
}

} // namespace pipeline
} // namespace roc
