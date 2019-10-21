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

const char* port_type_to_str(PortType type) {
    switch (type) {
    case Port_AudioSource:
        return "source";
    case Port_AudioRepair:
        return "repair";
    }
    return "?";
}

const char* port_proto_to_str(PortProtocol proto) {
    switch (proto) {
    case Proto_None:
        return "none";
    case Proto_RTP:
        return "rtp";
    case Proto_RTP_RSm8_Source:
        return "rtp+rs8m";
    case Proto_RSm8_Repair:
        return "rs8m";
    case Proto_RTP_LDPC_Source:
        return "rtp+ldpc";
    case Proto_LDPC_Repair:
        return "ldpc";
    }
    return "?";
}

port_to_str::port_to_str(const PortConfig& port) {
    buffer_[0] = '\0';

    if (snprintf(buffer_, sizeof(buffer_), "%s:%s", port_proto_to_str(port.protocol),
                 address::socket_addr_to_str(port.address).c_str())
        < 0) {
        roc_log(LogError, "port to str: can't format port");
    }
}

} // namespace pipeline
} // namespace roc
