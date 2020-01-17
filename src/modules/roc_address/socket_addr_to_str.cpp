/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/string_utils.h"

namespace roc {
namespace address {

socket_addr_to_str::socket_addr_to_str(const SocketAddr& addr) {
    switch ((unsigned)addr.version()) {
    case Family_IPv4:
        if (!format_ipv4_(addr)) {
            strcpy(buffer_, "<error>");
            return;
        }
        break;

    case Family_IPv6:
        if (!format_ipv6_(addr)) {
            strcpy(buffer_, "<error>");
            return;
        }
        break;
    default:
        strcpy(buffer_, "<none>");
        return;
    }

    if (addr.broadcast()) {
        if (!core::append_str(buffer_, sizeof(buffer_), " broadcast")) {
            strcpy(buffer_, "<error>");
            return;
        }
    }
}

bool socket_addr_to_str::format_ipv4_(const SocketAddr& addr) {
    if (!addr.get_host(buffer_, sizeof(buffer_))) {
        return false;
    }

    if (!core::append_str(buffer_, sizeof(buffer_), ":")) {
        return false;
    }

    if (!core::append_uint(buffer_, sizeof(buffer_), (uint64_t)addr.port(), 10)) {
        return false;
    }

    if (addr.has_miface()) {
        if (!core::append_str(buffer_, sizeof(buffer_), " miface ")) {
            return false;
        }

        const size_t blen = strlen(buffer_);

        if (!addr.get_miface(buffer_ + blen, sizeof(buffer_) - blen)) {
            return false;
        }
    }

    return true;
}

bool socket_addr_to_str::format_ipv6_(const SocketAddr& addr) {
    buffer_[0] = '[';

    if (!addr.get_host(buffer_ + 1, sizeof(buffer_) - 1)) {
        return false;
    }

    if (!core::append_str(buffer_, sizeof(buffer_), "]:")) {
        return false;
    }

    if (!core::append_uint(buffer_, sizeof(buffer_), (uint64_t)addr.port(), 10)) {
        return false;
    }

    if (addr.has_miface()) {
        if (!core::append_str(buffer_, sizeof(buffer_), " miface [")) {
            return false;
        }

        const size_t blen = strlen(buffer_);

        if (!addr.get_miface(buffer_ + blen, sizeof(buffer_) - blen)) {
            return false;
        }

        if (!core::append_str(buffer_, sizeof(buffer_), "]")) {
            return false;
        }
    }

    return true;
}

} // namespace address
} // namespace roc
