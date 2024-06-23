/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sdp/connection_data.h"

namespace roc {
namespace sdp {

ConnectionData::ConnectionData() {
}

void ConnectionData::clear() {
    connection_address_.clear();
}

bool ConnectionData::set_connection_address(address::AddrFamily addrtype,
                                            const char* str,
                                            size_t str_len) {
    char addr[address::SocketAddr::MaxStrLen];
    core::StringBuilder b(addr, sizeof(addr));

    if (!b.append_range(str, str + str_len)) {
        return false;
    }

    roc_log(LogInfo, "sdp: connection field address: %s", &addr[0]);

    if (!connection_address_.set_host_port(addrtype, addr, 0)) {
        return false;
    }

    return true;
}

const address::SocketAddr& ConnectionData::connection_address() const {
    return connection_address_;
}

} // namespace sdp
} // namespace roc
