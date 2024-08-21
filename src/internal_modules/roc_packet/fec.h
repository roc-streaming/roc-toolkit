/*
 * Copyright (c) 2017 Roc Streaming authors
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
#include "roc_packet/fec_scheme.h"
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! FECFRAME packet.
struct FEC {
    //! The FEC scheme to which the packet belongs to.
    //! @remarks
    //!  Defines both FEC header or footer format and FEC payload format.
    FecScheme fec_scheme;

    //! The index number of packet in a block ("esi").
    //! @remarks
    //!  Source packets are numbered in range [0; k).
    //!  Repair packets are numbered in range [k; k + n), where
    //!  k is a number of source packets per block (source_block_length)
    //!  n is a number of repair packets per block.
    size_t encoding_symbol_id;

    //! Number of a source block in a packet stream ("sbn").
    //! @remarks
    //!  Source block is formed from the source packets.
    //!  Blocks are numbered sequentially starting from a random number.
    //!  Block number can wrap.
    blknum_t source_block_number;

    //! Number of source packets in block to which this packet belongs ("sblen").
    //! @remarks
    //!  Different blocks can have different number of source packets.
    size_t source_block_length;

    //! Number of source + repair packets in block to which this packet belongs ("blen").
    //! @remarks
    //!  Different blocks can have different number of packets.
    //!  Always larger than source_block_length.
    //!  This field is not supported on all FEC schemes.
    size_t block_length;

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
