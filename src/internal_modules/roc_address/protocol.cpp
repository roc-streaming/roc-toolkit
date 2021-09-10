/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/protocol.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

const char* proto_to_str(Protocol proto) {
    switch (proto) {
    case Proto_None:
        break;

    case Proto_RTSP:
        return "rtsp";

    case Proto_RTP:
        return "rtp";

    case Proto_RTP_RS8M_Source:
        return "rtp+rs8m";

    case Proto_RS8M_Repair:
        return "rs8m";

    case Proto_RTP_LDPC_Source:
        return "rtp+ldpc";

    case Proto_LDPC_Repair:
        return "ldpc";
    }

    return NULL;
}

} // namespace address
} // namespace roc
