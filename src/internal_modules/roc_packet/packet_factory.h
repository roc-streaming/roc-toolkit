/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet_factory.h
//! @brief Packet factory.

#ifndef ROC_PACKET_PACKET_FACTORY_H_
#define ROC_PACKET_PACKET_FACTORY_H_

#include "roc_core/buffer.h"
#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/slab_pool.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Packet factory.
//!
//! Allows allocating packets and packet buffers (byte buffers of appropriate size
//! that should be attached to packet to hold payload).
//!
//! Serves several purposes:
//!  - implements convenient and type-safe wrapper on top of memory pools
//!  - combines two related pools (packet pool and buffer pool) in one class
//!  - detaches pipeline logic from memory management interface, so that it can
//!    change independently without affecting every pipeline element
class PacketFactory : public core::NonCopyable<> {
public:
    //! Initialize with default pools.
    //! @p buffer_size defines number of bytes in packet buffer.
    PacketFactory(core::IArena& arena, size_t buffer_size);

    //! Initialize with custom pools.
    //! @p packet_pool is a pool of packet::Packet objects.
    //! @p buffer_pool is a pool of core::Buffer objects.
    PacketFactory(core::IPool& packet_pool, core::IPool& buffer_pool);

    //! Get packet buffer size in bytes.
    size_t packet_buffer_size() const;

    //! Allocate packet buffer.
    //! @remarks
    //!  Returned buffer may be attached to packet using Packet::set_buffer().
    core::BufferPtr new_packet_buffer();

    //! Allocate packet.
    //! @remarks
    //!  Returned packet does not have a buffer, it should be allocated and
    //!  attached to the packet manually.
    PacketPtr new_packet();

private:
    // used if factory is created with default pools
    core::Optional<core::SlabPool<Packet> > default_packet_pool_;
    core::Optional<core::SlabPool<core::Buffer> > default_buffer_pool_;

    core::IPool* packet_pool_;
    core::IPool* buffer_pool_;
    size_t buffer_size_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_FACTORY_H_
