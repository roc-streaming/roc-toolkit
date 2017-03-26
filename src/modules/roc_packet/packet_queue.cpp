/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_core/helpers.h"

#include "roc_packet/packet_queue.h"

#define SEQ_IS_BEFORE(a, b) ROC_IS_BEFORE(signed_seqnum_t, a, b)

namespace roc {
namespace packet {

PacketQueue::PacketQueue(size_t max_size)
    : max_size_(max_size) {
}

IPacketConstPtr PacketQueue::read() {
    if (IPacketConstPtr packet = list_.back()) {
        list_.remove(*packet);
        return packet;
    }

    return NULL;
}

void PacketQueue::write(const IPacketConstPtr& packet) {
    if (!packet) {
        roc_panic("packet queue: attempting to add null packet");
    }

    if (max_size_ > 0 && list_.size() == max_size_) {
        roc_log(LogDebug, "packet queue: queue is full, dropping packet:"
                           " max_size=%u",
                (unsigned)max_size_);
        return;
    }

    IPacketConstPtr before = list_.front();

    for (; before; before = list_.next(*before)) {
        if (SEQ_IS_BEFORE(packet->seqnum(), before->seqnum())) {
            continue;
        }

        if (packet->seqnum() == before->seqnum()) {
            roc_log(LogDebug, "packet queue: dropping duplicate packet:"
                               " pkt_seqnum=%u",
                    (unsigned)packet->seqnum());
            return;
        }

        break;
    }

    if (before) {
        list_.insert(*packet, &*before);
    } else {
        list_.append(*packet);
    }
}

size_t PacketQueue::size() const {
    return list_.size();
}

IPacketConstPtr PacketQueue::head() const {
    return list_.back();
}

IPacketConstPtr PacketQueue::tail() const {
    return list_.front();
}

} // namespace packet
} // namespace roc
