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
    //! Number of a source block in a packet stream.
    //!
    //! @remarks
    //!  Source block is formed from the source packets.
    //!  Blocks are numbered sequentially starting from a random number.
    //!  Block number can wrap.
    blknum_t source_block_number;

    //! Number of source packets in the block to which this packet belongs to.
    //!
    //! @remarks
    //!  Different blocks can have different number of source packets.
    size_t source_block_length;

    //! The index number of packet in a block.
    //!
    //! @remarks
    //!  Source packets are numbered in range [0; k).
    //!  Repair packets are numbered in range [k; k + n), where
    //!  k is a number of source packets per block (source_block_length)
    //!  n is a number of repair packets per block.
    size_t encoding_symbol_id;

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
