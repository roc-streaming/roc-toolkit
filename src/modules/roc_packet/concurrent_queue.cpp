/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/concurrent_queue.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

ConcurrentQueue::ConcurrentQueue()
    : cond_(mutex_) {
}

PacketPtr ConcurrentQueue::read() {
    core::Mutex::Lock lock(mutex_);

    PacketPtr packet;
    while (!(packet = list_.front())) {
        cond_.wait();
    }

    list_.remove(*packet);

    return packet;
}

void ConcurrentQueue::write(const PacketPtr& packet) {
    if (!packet) {
        roc_panic("concurrent queue: packet is null");
    }

    core::Mutex::Lock lock(mutex_);

    list_.push_back(*packet);
    cond_.broadcast();
}

} // namespace packet
} // namespace roc
