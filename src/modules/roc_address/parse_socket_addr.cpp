/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/parse_socket_addr.h"

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

bool parse_socket_addr_miface(const char* input, SocketAddr& addr) {
    if (!input) {
        return false;
    }

    if (!addr.has_host_port()) {
        return false;
    }

    if (!addr.multicast()) {
        return false;
    }

    if (input[0] != '[') {
        if (addr.version() == 6) {
            return false;
        }

        return addr.set_miface(IPv4, input);
    }

    if (addr.version() == 4) {
        return false;
    }

    char addr6[SocketAddr::MaxStrLen] = {};

    if (!parse_ipv6_addr(input, input + strlen(input), addr6, sizeof(addr6))) {
        return false;
    }

    return addr.set_miface(IPv6, addr6);
}

} // namespace address
} // namespace roc
