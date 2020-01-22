/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_protocol.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

const char* endpoint_proto_to_str(EndpointProtocol proto) {
    switch (proto) {
    case EndProto_None:
        break;

    case EndProto_RTSP:
        return "rtsp";

    case EndProto_RTP:
        return "rtp";

    case EndProto_RTP_RS8M_Source:
        return "rtp+rs8m";

    case EndProto_RS8M_Repair:
        return "rs8m";

    case EndProto_RTP_LDPC_Source:
        return "rtp+ldpc";

    case EndProto_LDPC_Repair:
        return "ldpc";
    }

    return NULL;
}

} // namespace address
} // namespace roc
