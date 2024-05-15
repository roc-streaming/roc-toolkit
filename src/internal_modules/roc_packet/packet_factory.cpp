/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/packet_factory.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

PacketFactory::PacketFactory(core::IArena& arena, size_t buffer_size) {
    default_packet_pool_.reset(new (default_packet_pool_)
                                   core::SlabPool<Packet>("default_packet_pool", arena));

    default_buffer_pool_.reset(new (default_buffer_pool_) core::SlabPool<core::Buffer>(
        "default_packet_buffer_pool", arena, sizeof(core::Buffer) + buffer_size));

    packet_pool_ = default_packet_pool_.get();
    buffer_pool_ = default_buffer_pool_.get();
    buffer_size_ = buffer_size;
}

PacketFactory::PacketFactory(core::IPool& packet_pool, core::IPool& buffer_pool) {
    if (packet_pool.object_size() != sizeof(Packet)) {
        roc_panic("packet factory: unexpected packet_pool object size:"
                  " expected=%lu actual=%lu",
                  (unsigned long)sizeof(Packet),
                  (unsigned long)packet_pool.object_size());
    }

    if (buffer_pool.object_size() < sizeof(core::Buffer)) {
        roc_panic("packet factory: unexpected buffer_pool object size:"
                  " minimum=%lu actual=%lu",
                  (unsigned long)sizeof(core::Buffer),
                  (unsigned long)buffer_pool.object_size());
    }

    packet_pool_ = &packet_pool;
    buffer_pool_ = &buffer_pool;
    buffer_size_ = buffer_pool.object_size() - sizeof(core::Buffer);
}

size_t PacketFactory::packet_buffer_size() const {
    return buffer_size_;
}

core::BufferPtr PacketFactory::new_packet_buffer() {
    return new (*buffer_pool_) core::Buffer(*buffer_pool_, buffer_size_);
}

PacketPtr PacketFactory::new_packet() {
    return new (*packet_pool_) Packet(*packet_pool_);
}

} // namespace packet
} // namespace roc
