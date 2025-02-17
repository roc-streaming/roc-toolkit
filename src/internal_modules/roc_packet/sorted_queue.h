/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/sorted_queue.h
//! @brief Sorted packet queue.

#ifndef ROC_PACKET_SORTED_QUEUE_H_
#define ROC_PACKET_SORTED_QUEUE_H_

#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Sorted packet queue.
//! @remarks
//!  Packets order is determined by Packet::compare() method.
//! @note
//!  Not thread safe.
class SortedQueue : public IWriter, public IReader, public core::NonCopyable<> {
public:
    //! Construct empty queue.
    //! @remarks
    //!  If @p max_size is non-zero, it specifies maximum number of packets in queue.
    explicit SortedQueue(size_t max_size);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get number of packets in queue.
    size_t size() const;

    //! Get the latest packet that were ever added to the queue.
    //! @remarks
    //!  Returns null if the queue never had any packets. Otherwise, returns
    //!  the latest (by sorting order) ever added packet, even if that packet is not
    //!  currently in the queue. Returned packet is not removed from the queue if
    //!  it's still there.
    PacketPtr latest() const;

    //! Get first packet in the queue.
    //! @returns
    //!  the first packet in the queue or null if there are no packets
    //! @remarks
    //!  Returned packet is not removed from the queue.
    PacketPtr head() const;

    //! Get last packet in the queue.
    //! @returns
    //!  the last packet in the queue or null if there are no packets
    //! @remarks
    //!  Returned packet is not removed from the queue.
    PacketPtr tail() const;

    //! Add packet to the queue.
    //! @remarks
    //!  - if the maximum queue size is reached, packet is dropped
    //!  - if packet is equal to another packet in the queue, it is dropped
    //!  - otherwise, packet is inserted into the queue, keeping the queue sorted
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr& packet);

    //! Read next packet.
    //!
    //! @remarks
    //!  Removes returned packet from the queue.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr& packet,
                                                       PacketReadMode mode);

private:
    core::List<Packet> list_;
    PacketPtr latest_;
    const size_t max_size_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_SORTED_QUEUE_H_
