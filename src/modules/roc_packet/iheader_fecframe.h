/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/iheader_fecframe.h
//! @brief FECFRAME header interface.

#ifndef ROC_PACKET_IHEADER_FECFRAME_H_
#define ROC_PACKET_IHEADER_FECFRAME_H_

#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! FECFRAME header interface.
class IHeaderFECFrame {
public:
    virtual ~IHeaderFECFrame();

    //! Seqnum of first data packet in block.
    virtual seqnum_t data_blknum() const = 0;

    //! Set seqnum of first data packet in block.
    virtual void set_data_blknum(seqnum_t) = 0;

    //! Seqnum of first FEC packet in block.
    virtual seqnum_t fec_blknum() const = 0;

    //! Set seqnum of first FEC packet in block.
    virtual void set_fec_blknum(seqnum_t) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IHEADER_FECFRAME_H_
