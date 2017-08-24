/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/concurrent_queue.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

ConcurrentQueue::ConcurrentQueue(size_t max_size, bool blocking)
    : max_size_(max_size)
    , blocking_(blocking)
    , sem_(0) {
}

PacketPtr ConcurrentQueue::read() {
    if (blocking_) {
        sem_.pend();
    } else {
        if (!sem_.try_pend()) {
            return NULL;
        }
    }

    return read_nb_();
}

void ConcurrentQueue::write(const PacketPtr& packet) {
    if (!packet) {
        roc_panic("concurrent queue: null packet in write");
    }

    if (write_nb_(packet)) {
        sem_.post();
    }
}

void ConcurrentQueue::wait() {
    sem_.wait();
}

size_t ConcurrentQueue::size() const {
    core::Mutex::Lock lock(mutex_);

    return list_.size();
}

PacketPtr ConcurrentQueue::read_nb_() {
    core::Mutex::Lock lock(mutex_);

    PacketPtr packet = list_.front();
    if (!packet) {
        roc_panic("concurrent queue: null packet in read");
    }

    list_.remove(*packet);

    return packet;
}

bool ConcurrentQueue::write_nb_(const PacketPtr& packet) {
    core::Mutex::Lock lock(mutex_);

    if (max_size_ != 0 && list_.size() == max_size_) {
        roc_log(LogDebug, "concurrent queue: queue is full, dropping packet:"
                          " max_size=%u",
                (unsigned)max_size_);
        return false;
    }

    list_.push_back(*packet);

    return true;
}

} // namespace packet
} // namespace roc
