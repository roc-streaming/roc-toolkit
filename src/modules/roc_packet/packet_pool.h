/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet_pool.h
//! @brief Packet pool.

#ifndef ROC_PACKET_PACKET_POOL_H_
#define ROC_PACKET_PACKET_POOL_H_

#include "roc_core/pool.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Packet pool.
class PacketPool : public core::Pool<Packet> {
public:
    //! Constructor.
    PacketPool(core::IAllocator& allocator, size_t n_packets)
        : core::Pool<Packet>(allocator, sizeof(Packet), n_packets) {
    }
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_POOL_H_
