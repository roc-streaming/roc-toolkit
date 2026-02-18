/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/fifo_queue.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

status::StatusCode FifoQueue::init_status() const {
    return status::StatusOK;
}

size_t FifoQueue::size() const {
    return list_.size();
}

PacketPtr FifoQueue::head() const {
    return list_.front();
}

PacketPtr FifoQueue::tail() const {
    return list_.back();
}

status::StatusCode FifoQueue::write(const PacketPtr& packet) {
    if (!packet) {
        roc_panic("fifo queue: null packet");
    }

    list_.push_back(*packet);
    return status::StatusOK;
}

status::StatusCode FifoQueue::read(PacketPtr& packet, PacketReadMode mode) {
    packet = list_.front();
    if (!packet) {
        return status::StatusDrain;
    }

    if (mode == ModeFetch) {
        list_.remove(*packet);
    }
    return status::StatusOK;
}

} // namespace packet
} // namespace roc
