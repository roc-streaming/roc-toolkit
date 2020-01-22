/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/parse_port.h"
#include "roc_core/log.h"

namespace roc {
namespace pipeline {

namespace {

bool match_proto(address::EndpointType type,
                 const char* str,
                 address::EndpointProtocol& proto) {
    switch ((int)type) {
    case address::EndType_AudioSource:
        if (strcmp(str, "rtp") == 0) {
            proto = address::EndProto_RTP;
        } else if (strcmp(str, "rtp+rs8m") == 0) {
            proto = address::EndProto_RTP_RS8M_Source;
        } else if (strcmp(str, "rtp+ldpc") == 0) {
            proto = address::EndProto_RTP_LDPC_Source;
        } else {
            roc_log(LogError, "parse port: '%s' is not a valid source port protocol",
                    str);
            return false;
        }
        return true;

    case address::EndType_AudioRepair:
        if (strcmp(str, "rs8m") == 0) {
            proto = address::EndProto_RS8M_Repair;
        } else if (strcmp(str, "ldpc") == 0) {
            proto = address::EndProto_LDPC_Repair;
        } else {
            roc_log(LogError, "parse port: '%s' is not a valid repair port protocol",
                    str);
            return false;
        }
        return true;

    default:
        break;
    }

    roc_log(LogError, "parse port: unsupported port type");
    return false;
}

} // namespace

bool parse_port(address::EndpointType type, const char* input, PortConfig& result) {
    if (input == NULL) {
        roc_log(LogError, "parse port: string is null");
        return false;
    }

    const char* lcolon = strchr(input, ':');
    const char* rcolon = strrchr(input, ':');

    if (!lcolon || !rcolon || lcolon == rcolon || lcolon == input || !rcolon[1]) {
        roc_log(LogError,
                "parse port: bad format: expected PROTO:ADDR:PORT or PROTO::PORT");
        return false;
    }

    address::EndpointProtocol protocol = address::EndProto_None;

    char proto_buf[16] = {};
    if (size_t(lcolon - input) > sizeof(proto_buf) - 1) {
        roc_log(LogError, "parse port: bad protocol: too long");
        return false;
    }
    memcpy(proto_buf, input, size_t(lcolon - input));
    proto_buf[lcolon - input] = '\0';

    if (!match_proto(type, proto_buf, protocol)) {
        return false;
    }

    const char* addr = NULL;

    char addr_buf[256] = {};
    if (size_t(rcolon - lcolon) > sizeof(addr_buf) - 1) {
        roc_log(LogError, "parse port: bad address: too long");
        return false;
    }

    if (rcolon > lcolon + 1) {
        memcpy(addr_buf, lcolon + 1, size_t(rcolon - lcolon - 1));
        addr_buf[rcolon - lcolon - 1] = '\0';
        addr = addr_buf;
    } else {
        addr = "0.0.0.0";
    }

    const char* port = &rcolon[1];

    if (!isdigit(*port)) {
        roc_log(LogError, "parse port: bad port: not a number");
        return false;
    }

    char* port_end = NULL;
    long port_num = strtol(port, &port_end, 10);

    if (port_num == LONG_MAX || port_num == LONG_MIN || !port_end || *port_end) {
        roc_log(LogError, "parse port: bad port: not a positive integer");
        return false;
    }

    if (port_num < 0 || port_num > 65535) {
        roc_log(LogError, "parse port: bad port: not in range [1; 65535]");
        return false;
    }

    if (addr[0] == '[') {
        size_t addrlen = strlen(addr);
        if (addr[addrlen - 1] != ']') {
            roc_log(LogError, "parse port: bad IPv6 address: expected closing ']'");
            return false;
        }

        char addr6[address::SocketAddr::MaxStrLen] = {};
        if (addrlen - 2 > sizeof(addr6) - 1) {
            roc_log(LogError, "parse port: bad IPv6 address: address too long");
            return false;
        }

        memcpy(addr6, addr + 1, addrlen - 2);

        if (!result.address.set_host_port_ipv6(addr6, (int)port_num)) {
            roc_log(LogError, "parse port: bad IPv6 address: %s", addr6);
            return false;
        }
    } else {
        if (!result.address.set_host_port_ipv4(addr, (int)port_num)) {
            roc_log(LogError, "parse port: bad IPv4 address: %s", addr);
            return false;
        }
    }

    result.protocol = protocol;

    return true;
}

} // namespace pipeline
} // namespace roc
