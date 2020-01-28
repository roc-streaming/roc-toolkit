/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace address {

socket_addr_to_str::socket_addr_to_str(const SocketAddr& addr) {
    if (!format_(addr)) {
        strcpy(buffer_, "<none>");
    }
}

bool socket_addr_to_str::format_(const SocketAddr& addr) {
    core::StringBuilder b(buffer_, sizeof(buffer_));

    char ip[64];

    if (addr.family() == Family_IPv4) {
        if (!addr.get_host(ip, sizeof(ip))) {
            return false;
        }

        b.append_str(ip);
        b.append_str(":");
        b.append_uint((uint64_t)addr.port(), 10);

        if (addr.has_miface()) {
            if (!addr.get_miface(ip, sizeof(ip))) {
                return false;
            }
            b.append_str(" miface ");
            b.append_str(ip);
        }

        if (addr.broadcast()) {
            b.append_str(" broadcast");
        }

        return true;
    }

    if (addr.family() == Family_IPv6) {
        if (!addr.get_host(ip, sizeof(ip))) {
            return false;
        }

        b.append_str("[");
        b.append_str(ip);
        b.append_str("]:");
        b.append_uint((uint64_t)addr.port(), 10);

        if (addr.has_miface()) {
            if (!addr.get_miface(ip, sizeof(ip))) {
                return false;
            }
            b.append_str(" miface [");
            b.append_str(ip);
            b.append_str("]");
        }

        if (addr.broadcast()) {
            b.append_str(" broadcast");
        }

        return true;
    }

    return false;
}

} // namespace address
} // namespace roc
