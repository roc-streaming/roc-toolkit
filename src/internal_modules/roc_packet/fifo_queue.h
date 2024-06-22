/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/fifo_queue.h
//! @brief Packet FIFO queue.

#ifndef ROC_PACKET_FIFO_QUEUE_H_
#define ROC_PACKET_FIFO_QUEUE_H_

#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Packet FIFO queue.
//! @remarks
//!  Packets order is not changed.
//! @note
//!  Not thread safe.
class FifoQueue : public IWriter, public IReader, public core::NonCopyable<> {
public:
    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get number of packets in queue.
    size_t size() const;

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
    //!  Adds packet to the end of the queue.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr& packet);

    //! Read next packet.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr& packet,
                                                       PacketReadMode mode);

private:
    core::List<Packet> list_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_FIFO_QUEUE_H_
