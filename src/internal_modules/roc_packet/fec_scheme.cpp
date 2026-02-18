/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/fec_scheme.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace packet {

const char* fec_scheme_to_str(FecScheme scheme) {
    switch (scheme) {
    case FEC_None:
        return "none";
    case FEC_ReedSolomon_M8:
        return "rs8m";
    case FEC_LDPC_Staircase:
        return "ldpc";
    }
    return "?";
}

FecScheme fec_scheme_from_str(const char* str) {
    if (strcmp(str, "rs8m") == 0) {
        return FEC_ReedSolomon_M8;
    }
    if (strcmp(str, "ldpc") == 0) {
        return FEC_LDPC_Staircase;
    }
    return FEC_None;
}

} // namespace packet
} // namespace roc
