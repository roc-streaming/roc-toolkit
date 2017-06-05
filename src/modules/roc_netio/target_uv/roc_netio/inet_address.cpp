/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/byte_order.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

#include "roc_datagram/address_to_str.h"
#include "roc_netio/inet_address.h"

namespace roc {
namespace netio {

void to_inet_address(const datagram::Address& addr, sockaddr_in& sa) {
    const uint32_t ip                  //
        = ((uint32_t)addr.ip[0] << 24) //
        | ((uint32_t)addr.ip[1] << 16) //
        | ((uint32_t)addr.ip[2] << 8)  //
        | ((uint32_t)addr.ip[3]);

    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = ROC_HTON_16(addr.port);
    sa.sin_addr.s_addr = ROC_HTON_32(ip);
}

void from_inet_address(const sockaddr_in& sa, datagram::Address& addr) {
    const uint32_t ip = ROC_NTOH_32(sa.sin_addr.s_addr);

    addr.ip[0] = uint8_t((ip >> 24) & 0xff);
    addr.ip[1] = uint8_t((ip >> 16) & 0xff);
    addr.ip[2] = uint8_t((ip >> 8) & 0xff);
    addr.ip[3] = uint8_t((ip & 0xff));

    addr.port = ROC_NTOH_16(sa.sin_port);
}

bool parse_address(const char* input, datagram::Address& result) {
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
        roc_log(LogError, "parse address: string is not in form '<IP>:<PORT>'");
        return false;
    }

    if (!port) {
        roc_log(LogError, "parse address: bad port, expected non-empty string");
        return false;
    }

    if (!isdigit(*port)) {
        roc_log(LogError, "parse address: bad port, expected number");
        return false;
    }

    char* port_end = NULL;
    long port_num = strtol(port, &port_end, 10);

    if (port_num == LONG_MAX || port_num == LONG_MIN || !port_end || *port_end) {
        roc_log(LogError, "parse address: bad port, expected positive integer");
        return false;
    }

    if (port_num < 1 || port_num > 65535) {
        roc_log(LogError, "parse address: bad port, expected [1; 65535]");
        return false;
    }

    if (addr) {
        sockaddr_in sa;
        if (int err = uv_ip4_addr(addr, (int)port_num, &sa)) {
            roc_log(LogError, "parse address: uv_ip4_addr(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
            return false;
        }
        from_inet_address(sa, result);
    } else {
        result = datagram::Address();
        result.port = (datagram::port_t)port_num;
    }

    roc_log(LogDebug, "parse address: parsed %s",
            datagram::address_to_str(result).c_str());

    return true;
}

} // namespace netio
} // namespace roc
