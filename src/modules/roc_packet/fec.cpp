/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/fec.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

FEC::FEC()
    : fec_scheme(FEC_None)
    , encoding_symbol_id(0)
    , source_block_number(0)
    , source_block_length(0)
    , block_length(0) {
}

int FEC::compare(const FEC& other) const {
    if (blknum_lt(source_block_number, other.source_block_number)) {
        return -1;
    } else if (source_block_number == other.source_block_number) {
        if (encoding_symbol_id < other.encoding_symbol_id) {
            return -1;
        } else if (encoding_symbol_id == other.encoding_symbol_id) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

} // namespace packet
} // namespace roc
