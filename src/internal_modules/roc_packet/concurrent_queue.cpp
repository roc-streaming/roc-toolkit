/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/concurrent_queue.h"
#include "roc_core/panic.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

ConcurrentQueue::ConcurrentQueue(Mode mode) {
    if (mode == Blocking) {
        write_sem_.reset(new (write_sem_) core::Semaphore());
    }
}

status::StatusCode ConcurrentQueue::init_status() const {
    return status::StatusOK;
}

status::StatusCode ConcurrentQueue::write(const PacketPtr& packet) {
    if (!packet) {
        roc_panic("concurrent queue: packet is null");
    }

    queue_.push_back(*packet);

    if (write_sem_) {
        write_sem_->post();
    }

    return status::StatusOK;
}

status::StatusCode ConcurrentQueue::read(PacketPtr& packet, PacketReadMode mode) {
    core::Mutex::Lock lock(read_mutex_);

    if (!read_pkt_) {
        if (write_sem_) {
            write_sem_->wait();
        }

        if (!(read_pkt_ = queue_.pop_front_exclusive())) {
            return status::StatusDrain;
        }
    }

    packet = read_pkt_;
    if (mode == ModeFetch) {
        read_pkt_.reset();
    }

    return status::StatusOK;
}

} // namespace packet
} // namespace roc
