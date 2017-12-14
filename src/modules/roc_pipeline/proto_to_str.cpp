/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/proto_to_str.h"

namespace roc {
namespace pipeline {

const char* proto_to_str(Protocol proto) {
    switch (proto) {
    case Proto_None:
        return "none";
    case Proto_RTP:
        return "rtp";
    case Proto_RTP_RSm8_Source:
        return "rtp_rsm8_source";
    case Proto_RSm8_Repair:
        return "rsm8_repair";
    case Proto_RTP_LDPC_Source:
        return "rtp_ldpc_source";
    case Proto_LDPC_Repair:
        return "ldpc_repair";
    }
    return "invalid";
}

} // namespace pipeline
} // namespace roc
