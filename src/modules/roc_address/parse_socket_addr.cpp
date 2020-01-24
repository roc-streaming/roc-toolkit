/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/parse_socket_addr.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

namespace {

bool parse_ipv6_addr(const char* begin, const char* end, char* buf, size_t bufsz) {
    if (begin == end) {
        return false;
    }

    if (begin + 1 == end) {
        return false;
    }

    if (*begin != '[') {
        return false;
    }

    if (*(end - 1) != ']') {
        return false;
    }

    if (size_t(end - begin) - 2 > bufsz - 1) {
        return false;
    }

    memcpy(buf, begin + 1, size_t(end - begin) - 2);
    buf[size_t(end - begin) - 2] = '\0';

    return true;
}

} // namespace

bool parse_socket_addr_host_port(const char* host, int port, SocketAddr& addr) {
    roc_panic_if(!host);

    if (port < 0) {
        return false;
    }

    if (host[0] == '[') {
        char addr6[SocketAddr::MaxStrLen] = {};

        if (!parse_ipv6_addr(host, host + strlen(host), addr6, sizeof(addr6))) {
            return false;
        }

        if (!addr.set_host_port(Family_IPv6, addr6, port)) {
            return false;
        }

        return true;
    } else {
        if (!addr.set_host_port(Family_IPv4, host, port)) {
            return false;
        }

        return true;
    }
}

bool parse_socket_addr_miface(const char* miface, SocketAddr& addr) {
    roc_panic_if(!miface);

    if (!addr.has_host_port()) {
        return false;
    }

    if (!addr.multicast()) {
        return false;
    }

    if (miface[0] == '[') {
        if (addr.family() != Family_IPv6) {
            return false;
        }

        char addr6[SocketAddr::MaxStrLen] = {};

        if (!parse_ipv6_addr(miface, miface + strlen(miface), addr6, sizeof(addr6))) {
            return false;
        }

        if (!addr.set_miface(Family_IPv6, addr6)) {
            return false;
        }

        return true;
    } else {
        if (addr.family() != Family_IPv4) {
            return false;
        }

        if (!addr.set_miface(Family_IPv4, miface)) {
            return false;
        }

        return true;
    }
}

} // namespace address
} // namespace roc
