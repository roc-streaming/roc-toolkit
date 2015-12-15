/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ipacket.h
//! @brief Packet interface.

#ifndef ROC_PACKET_IPACKET_H_
#define ROC_PACKET_IPACKET_H_

#include "roc_core/byte_buffer.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_core/shared_ptr.h"

#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! Packet type.
typedef const void* PacketType;

//! Packet interface.
class IPacket : public core::RefCnt, public core::ListNode {
public:
    virtual ~IPacket();

    //! Get packet type.
    //! @remarks
    //!  Each derived interface (e.g., IAudioPacket), but not implementation
    //!  (e.g. rtp::AudioPacket) returns it's own unique identifier.
    virtual PacketType type() const = 0;

    //! Get packet sequence number in client stream.
    //! @remarks
    //!  Packets are numbered sequentaly in every stream, starting from some
    //!  random value.
    //! @note
    //!  Seqnum overflow may occur, use ROC_IS_BEFORE() macro to compare them.
    virtual seqnum_t seqnum() const = 0;

    //! Set packet sequence number.
    virtual void set_seqnum(seqnum_t) = 0;

    //! Get packet marker bit.
    //! @remarks
    //!  Marker bit can be interpreted differently by dirrerent modules.
    virtual bool marker() const = 0;

    //! Set packet marker bit.
    virtual void set_marker(bool) = 0;

    //! Get packet data buffer (containing header and payload).
    //! @remarks
    //!  Never returns empty slice.
    virtual core::IByteBufferConstSlice raw_data() const = 0;

    //! Print packet to stdout.
    virtual void print(bool body = false) const = 0;
};

//! Packet smart pointer.
typedef core::SharedPtr<IPacket> IPacketPtr;

//! Const packet smart pointer.
typedef core::SharedPtr<IPacket const> IPacketConstPtr;

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPACKET_H_
