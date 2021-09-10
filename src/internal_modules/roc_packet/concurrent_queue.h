/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/concurrent_queue.h
//! @brief Concurrent blocking packet queue.

#ifndef ROC_PACKET_CONCURRENT_QUEUE_H_
#define ROC_PACKET_CONCURRENT_QUEUE_H_

#include "roc_core/cond.h"
#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Concurrent blocking packet queue.
class ConcurrentQueue : public IReader, public IWriter, public core::NonCopyable<> {
public:
    ConcurrentQueue();

    //! Read next packet.
    //! @remarks
    //!  Blocks until the queue becomes non-empty and returns the first
    //!  packet from the queue.
    virtual PacketPtr read();

    //! Add packet to the queue.
    //! @remarks
    //!  Adds packet to the end of the queue.
    virtual void write(const PacketPtr& packet);

private:
    core::Mutex mutex_;
    core::Cond cond_;
    core::List<Packet> list_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_CONCURRENT_QUEUE_H_
