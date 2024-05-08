/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/concurrent_queue.h
//! @brief Concurrent blocking packet queue.

#ifndef ROC_PACKET_CONCURRENT_QUEUE_H_
#define ROC_PACKET_CONCURRENT_QUEUE_H_

#include "roc_core/mpsc_queue.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/semaphore.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Concurrent blocking packet queue.
class ConcurrentQueue : public IReader, public IWriter, public core::NonCopyable<> {
public:
    //! Queue mode.
    enum Mode {
        Blocking,   //!< Read operation blocks until queue is non-empty.
        NonBlocking //!< Read operation returns null if queue is empty.
    };

    //! Initialize.
    //! @p mode defines whether reads will be blocking.
    explicit ConcurrentQueue(Mode mode);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read next packet.
    //! If reads are not concurrent, and queue is non-blocking, then
    //! reads are wait-free. Otherwise they may block.
    //! @see Mode.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr& packet);

    //! Add packet to the queue.
    //! Wait-free operation.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr& packet);

private:
    core::Optional<core::Semaphore> write_sem_;
    core::Mutex read_mutex_;
    core::MpscQueue<Packet> queue_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_CONCURRENT_QUEUE_H_
