/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/shared_ptr.h"

#include "roc_rtp/composer.h"
#include "roc_rtp/packet.h"

namespace roc {
namespace rtp {

Composer::Composer(core::IPool<AudioPacket>& audio_pool,
                   core::IPool<ContainerPacket>& container_pool,
                   core::IByteBufferComposer& buffer_composer)
    : audio_pool_(audio_pool)
    , container_pool_(container_pool)
    , buffer_composer_(buffer_composer) {
}

packet::IPacketPtr Composer::compose(int options) {
    core::IByteBufferPtr buffer = buffer_composer_.compose();
    if (!buffer) {
        roc_log(LogError, "rtp composer: can't allocate buffer");
        return NULL;
    }

    core::SharedPtr<Packet> packet;

    if (options & packet::IPacket::HasAudio) {
        packet = new (audio_pool_) AudioPacket(audio_pool_, NULL);
    }

    if (options & packet::IPacket::HasFEC) { // FIXME
        packet = new (container_pool_) ContainerPacket(container_pool_);
    }

    if (!packet) {
        roc_panic("rtp composer: bad options");
    }

    packet->compose(buffer);

    // FIXME
    if (packet->fec()) {
        packet->header().set_payload_type(123);
    }

    return packet;
}

} // namespace rtp
} // namespace roc
