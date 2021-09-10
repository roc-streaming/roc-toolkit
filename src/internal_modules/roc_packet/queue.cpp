/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/queue.h"

namespace roc {
namespace packet {

PacketPtr Queue::read() {
    PacketPtr packet = list_.front();
    if (!packet) {
        return NULL;
    }
    list_.remove(*packet);
    return packet;
}

void Queue::write(const PacketPtr& packet) {
    if (!packet) {
        roc_panic("queue: null packet");
    }
    list_.push_back(*packet);
}

size_t Queue::size() const {
    return list_.size();
}

} // namespace packet
} // namespace roc
