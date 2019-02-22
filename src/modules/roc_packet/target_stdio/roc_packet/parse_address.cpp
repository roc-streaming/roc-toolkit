/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/parse_address.h"
#include "roc_core/log.h"
#include "roc_packet/address_to_str.h"

namespace roc {
namespace packet {

bool parse_address(const char* input, Address& result) {
    if (input == NULL) {
        roc_log(LogError, "parse address: string is null");
        return false;
    }

    const char* addr = NULL;
    const char* port = NULL;

    char buf[256] = {};

    if (const char* colon = strrchr(input, ':')) {
        if (size_t(colon - input) > sizeof(buf) - 1) {
            roc_log(LogError, "parse address: string is too long");
            return false;
        }
        if (colon > input) {
            memcpy(buf, input, size_t(colon - input));
            buf[colon - input] = '\0';
            addr = buf;
        }
        port = colon[1] ? &colon[1] : NULL;
    } else {
        roc_log(LogError, "parse address: string is not in form '[IP]:PORT'");
        return false;
    }

    if (!port) {
        roc_log(LogError, "parse address: bad port, expected non-empty string");
        return false;
    }

    if (!isdigit(*port)) {
        roc_log(LogError, "parse address: bad port, expected a number");
        return false;
    }

    char* port_end = NULL;
    long port_num = strtol(port, &port_end, 10);

    if (port_num == LONG_MAX || port_num == LONG_MIN || !port_end || *port_end) {
        roc_log(LogError, "parse address: bad port, expected positive integer");
        return false;
    }

    if (port_num < 0 || port_num > 65535) {
        roc_log(LogError, "parse address: bad port, expected [1; 65535]");
        return false;
    }

    if (!addr) {
        addr = "0.0.0.0";
    }

    if (addr[0] == '[') {
        size_t addrlen = strlen(addr);
        if (addr[addrlen - 1] != ']') {
            roc_log(LogError, "parse address: bad IPv6 address: expected closing ']'");
            return false;
        }

        char addr6[128] = {};
        if (addrlen - 2 > sizeof(addr6) - 1) {
            roc_log(LogError, "parse address: bad IPv6 address: address too long");
            return false;
        }

        memcpy(addr6, addr + 1, addrlen - 2);

        if (!result.set_ipv6(addr6, (int)port_num)) {
            roc_log(LogError, "parse address: bad IPv6 address: %s", addr6);
            return false;
        }
    } else {
        if (!result.set_ipv4(addr, (int)port_num)) {
            roc_log(LogError, "parse address: bad IPv4 address: %s", addr);
            return false;
        }
    }

    return true;
}

} // namespace packet
} // namespace roc
