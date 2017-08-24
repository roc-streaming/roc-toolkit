/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/concurrent_queue.h
//! @brief Concurrent packet queue.

#ifndef ROC_PACKET_CONCURRENT_QUEUE_H_
#define ROC_PACKET_CONCURRENT_QUEUE_H_

#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/semaphore.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Concurrent packet queue.
//! @remarks
//!  A thread-safe fifo.
class ConcurrentQueue : public IReader, public IWriter, public core::NonCopyable<> {
public:
    //! Construct queue.
    //!
    //! @b Parameters
    //!  - if @p max_size is non-zero, it specifies maximum number of
    //!    packets in queue
    //!  - if @p blocking is true, read() blocks until the queue becomes
    //!    non-empty instead of returning null immediately
    ConcurrentQueue(size_t max_size, bool blocking);

    //! Read next packet.
    //! @returns
    //!  the first packet in the queue or null if there are no packets
    //!  and the queue is non-blocking
    //! @remarks
    //!  Removes returned packet from the queue.
    virtual PacketPtr read();

    //! Add packet to the queue.
    //! @remarks
    //!  Adds packet to the end of the queue.
    virtual void write(const PacketPtr& packet);

    //! Wait until the queue becomes non-empty.
    //! @remarks
    //!  It's not guaranteed that the queue is still non empty when the
    //!  function returns if there are other threads that may call read().
    void wait();

    //! Get number of packets in queue.
    size_t size() const;

private:
    PacketPtr read_nb_();
    bool write_nb_(const PacketPtr& packet);

    const size_t max_size_;
    const bool blocking_;

    core::Semaphore sem_;
    core::Mutex mutex_;
    core::List<Packet> list_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_CONCURRENT_QUEUE_H_
