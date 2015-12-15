/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ifec_packet.h
//! @brief FEC packet interface.

#ifndef ROC_PACKET_IFEC_PACKET_H_
#define ROC_PACKET_IFEC_PACKET_H_

#include "roc_packet/ipacket.h"

namespace roc {
namespace packet {

//! FEC packet interface.
class IFECPacket : public IPacket {
public:
    virtual ~IFECPacket();

    //! Seqnum of first data packet in block.
    virtual seqnum_t data_blknum() const = 0;

    //! Set seqnum of first data packet in block.
    virtual void set_data_blknum(seqnum_t) = 0;

    //! Seqnum of first FEC packet in block.
    virtual seqnum_t fec_blknum() const = 0;

    //! Set seqnum of first FEC packet in block.
    virtual void set_fec_blknum(seqnum_t) = 0;

    //! Get payload.
    //! @remarks
    //!  Contains encoded symbols.
    //! @note
    //!  returned object hold reference to underlying packet buffer.
    virtual core::IByteBufferConstSlice payload() const = 0;

    //! Set payload data and size.
    //! @remarks
    //!  Copy @p size bytes from @p data of payload to packet's buffer.
    virtual void set_payload(const uint8_t* data, size_t size) = 0;

    //! FEC packet type.
    static const PacketType Type;

    //! Get packet type.
    virtual PacketType type() const {
        return Type;
    }

    //! Print packet to stdout.
    virtual void print(bool body = false) const;
};

//! FEC packet smart pointer.
typedef core::SharedPtr<IFECPacket> IFECPacketPtr;

//! Const FEC packet smart pointer.
typedef core::SharedPtr<IFECPacket const> IFECPacketConstPtr;

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IFEC_PACKET_H_
