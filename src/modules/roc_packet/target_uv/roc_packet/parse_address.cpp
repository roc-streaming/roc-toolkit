/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <uv.h>

#include "roc_core/log.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/parse_address.h"

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
            roc_log(LogError, "parse address: bad ipv6, expected closing ']'");
            return false;
        }

        char addr6[128] = {};
        if (addrlen - 2 > sizeof(addr6) - 1) {
            roc_log(LogError, "parse address: bad ipv6, address too long");
            return false;
        }

        memcpy(addr6, addr + 1, addrlen - 2);

        if (int err = uv_ip6_addr(addr6, (int)port_num, (sockaddr_in6*)result.saddr())) {
            roc_log(LogError, "parse address: uv_ip6_addr(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
            return false;
        }
    } else {
        if (int err = uv_ip4_addr(addr, (int)port_num, (sockaddr_in*)result.saddr())) {
            roc_log(LogError, "parse address: uv_ip4_addr(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
            return false;
        }
    }

    roc_log(LogDebug, "parse address: parsed %s", address_to_str(result).c_str());
    return true;
}

} // namespace packet
} // namespace roc
