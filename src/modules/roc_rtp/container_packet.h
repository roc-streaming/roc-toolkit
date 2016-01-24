/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/container_packet.h
//! @brief RTP container packet.

#ifndef ROC_RTP_CONTAINER_PACKET_H_
#define ROC_RTP_CONTAINER_PACKET_H_

#include "roc_core/noncopyable.h"
#include "roc_core/ipool.h"
#include "roc_packet/ipacket.h"
#include "roc_rtp/packet.h"

namespace roc {
namespace rtp {

//! RTP container packet.
//! @remarks
//!  Contains inner (encapsulated) packet in RTP payload.
class ContainerPacket : public Packet, private packet::IHeaderFECFrame { // FIXME
public:
    //! Initialize.
    ContainerPacket(core::IPool<ContainerPacket>&); // TODO

    //! Get packet options.
    virtual int options() const;

    //! Get FECFRAME header.
    virtual const packet::IHeaderFECFrame* fec() const;

    //! Get FECFRAME header.
    virtual packet::IHeaderFECFrame* fec();

private:
    virtual void free();

    // FIXME
    virtual packet::seqnum_t data_blknum() const;
    virtual void set_data_blknum(packet::seqnum_t);

    virtual packet::seqnum_t fec_blknum() const;
    virtual void set_fec_blknum(packet::seqnum_t);

    // TODO
    core::IPool<ContainerPacket>& pool_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_CONTAINER_PACKET_H_
