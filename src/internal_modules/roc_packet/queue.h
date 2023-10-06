/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/queue.h
//! @brief Packet queue.

#ifndef ROC_PACKET_QUEUE_H_
#define ROC_PACKET_QUEUE_H_

#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Packet queue.
class Queue : public IReader, public IWriter, public core::NonCopyable<> {
public:
    //! Read next packet.
    virtual status::StatusCode read(PacketPtr& packet);

    //! Add packet to the queue.
    //! @remarks
    //!  Adds packet to the end of the queue.
    virtual void write(const PacketPtr& packet);

    //! Get number of packets in queue.
    size_t size() const;

private:
    core::List<Packet> list_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_QUEUE_H_
