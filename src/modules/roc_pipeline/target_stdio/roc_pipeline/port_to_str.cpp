/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_address/socket_addr_to_str.h"
#include "roc_pipeline/port_to_str.h"

namespace roc {
namespace pipeline {

port_to_str::port_to_str(const PortConfig& port) {
    buffer_[0] = '\0';

    if (snprintf(buffer_, sizeof(buffer_), "%s:%s",
                 address::endpoint_proto_to_str(port.protocol),
                 address::socket_addr_to_str(port.address).c_str())
        < 0) {
        roc_log(LogError, "port to str: can't format port");
    }
}

} // namespace pipeline
} // namespace roc
