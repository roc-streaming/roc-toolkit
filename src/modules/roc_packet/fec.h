/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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
    seqnum_t source_blknum;

    //! Seqnum of first repair packet in block.
    seqnum_t repair_blknum;

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
