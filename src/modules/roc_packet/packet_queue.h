/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet_queue.h
//! @brief Sorted packet queue.

#ifndef ROC_PACKET_PACKET_QUEUE_H_
#define ROC_PACKET_PACKET_QUEUE_H_

#include "roc_core/list.h"
#include "roc_core/noncopyable.h"

#include "roc_packet/ipacket.h"
#include "roc_packet/ipacket_reader.h"
#include "roc_packet/ipacket_writer.h"

namespace roc {
namespace packet {

//! Sorted packet queue.
//! @note
//!  To handle seqnum overflow, ROC_IS_BEFORE() macro is used to compare seqnums.
class PacketQueue : public IPacketReader,
                    public IPacketConstWriter,
                    public core::NonCopyable<> {
public:
    //! Construct empty queue.
    //! @remarks
    //!  If @p max_size is non-zero, it specifies maximum number of
    //!  packets in queue.
    PacketQueue(size_t max_size = 0);

    //! Read next packet.
    //! @returns
    //!  packet with minimum seqnum or NULL if there are no packets.
    //! @remarks
    //!  Removes returned packet from the queue.
    virtual IPacketConstPtr read();

    //! Add packet to the queue.
    //! @remarks
    //!  - if maximum queue size is reached, packet is dropped;
    //!  - if packet's seqnum is equal to seqnum of some another packet in
    //!    queue, it is dropped;
    //!  - otherwise, packet is inserted into the queue sorted by packet's
    //!    seqnums.
    virtual void write(const IPacketConstPtr& packet);

    //! Get number of packets in queue.
    size_t size() const;

    //! Get first packet in the queue.
    //! @returns
    //!  packet with minimum seqnum or NULL if there are no packets.
    //! @remarks
    //!  Returned packet is *not* removed from the queue.
    IPacketConstPtr head() const;

    //! Get last packet in the queue.
    //! @returns
    //!  packet with maximum seqnum or NULL if there are no packets.
    //! @remarks
    //!  Returned packet is *not* removed from the queue.
    IPacketConstPtr tail() const;

private:
    core::List<const IPacket> list_;
    const size_t max_size_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_QUEUE_H_
