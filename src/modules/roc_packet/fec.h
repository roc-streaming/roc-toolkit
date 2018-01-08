/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/fec.h
//! @brief FEC packet.

#ifndef ROC_PACKET_FEC_H_
#define ROC_PACKET_FEC_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! FECFRAME packet.
struct FEC {
    //! Seqnum of first source packet in block.
    seqnum_t blknum;

    //! The number of source packet per block.
    size_t source_block_length;

    //! The index number of the repair packet in block.
    //!
    //! Must be source_block_length < repair_symbol_id.
    size_t repair_symbol_id;

    //! FECFRAME header or footer.
    core::Slice<uint8_t> payload_id;

    //! FECFRAME payload.
    //! @remarks
    //!  Doesn't include FECFRAME header or footer.
    core::Slice<uint8_t> payload;

    //! Construct zero FEC packet.
    FEC();

    //! Determine packet order.
    int compare(const FEC&) const;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_FEC_H_
