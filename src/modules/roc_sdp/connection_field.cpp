/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sdp/connection_field.h"

namespace roc {
namespace sdp {

ConnectionField::ConnectionField(core::IAllocator& allocator)
    : allocator_(allocator) {
    clear();
}

void ConnectionField::clear() {
    connection_address_.clear();
}

bool ConnectionField::set_connection_address(address::AddrFamily addrtype, const char* str, size_t str_len) {
    char addr[address::SocketAddr::MaxStrLen];
    core::StringBuilder b(addr, sizeof(addr));

    if (!b.append_str_range(str, str + str_len))
        return false;

    roc_log(LogInfo, "Connection Field address: %s", &addr[0]);

    if (!connection_address_.set_host_port(addrtype, addr, 0))
        return false;

    return true;
}

void ConnectionField::destroy() {
    allocator_.destroy(*this);
}

} // namespace roc
} // namespace sdp
