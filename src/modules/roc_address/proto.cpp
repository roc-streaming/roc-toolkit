/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/proto.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

packet::FecScheme proto_fec_scheme(EndpointProtocol proto) {
    switch ((int)proto) {
    case EndProto_RTP:
        return packet::FEC_None;

    case EndProto_RTP_RS8M_Source:
        return packet::FEC_ReedSolomon_M8;

    case EndProto_RS8M_Repair:
        return packet::FEC_ReedSolomon_M8;

    case EndProto_RTP_LDPC_Source:
        return packet::FEC_LDPC_Staircase;

    case EndProto_LDPC_Repair:
        return packet::FEC_LDPC_Staircase;
    }

    return packet::FEC_None;
}

int proto_default_port(EndpointProtocol proto) {
    switch ((int)proto) {
    case EndProto_RTSP:
        return 554;

    default:
        break;
    }

    return -1;
}

bool proto_supports_path(EndpointProtocol proto) {
    switch ((int)proto) {
    case EndProto_RTSP:
        return true;

    default:
        break;
    }

    return false;
}

const char* proto_to_str(EndpointProtocol proto) {
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
