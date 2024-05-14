/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/packet_factory.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

PacketFactory::PacketFactory(core::IArena& arena)
    : pool_("packet_pool", arena) {
}

PacketPtr PacketFactory::new_packet() {
    return new (pool_) Packet(pool_);
}

} // namespace packet
} // namespace roc
