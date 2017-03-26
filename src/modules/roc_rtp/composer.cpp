/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_rtp/rtp_packet.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace rtp {

Composer::Composer(core::IPool<AudioPacket>& audio_pool,
                   core::IPool<FECPacket>& fec_pool,
                   core::IByteBufferComposer& buffer_composer)
    : audio_pool_(audio_pool)
    , fec_pool_(fec_pool)
    , buffer_composer_(buffer_composer) {
}

packet::IPacketPtr Composer::compose(packet::PacketType type) {
    core::IByteBufferPtr buffer = buffer_composer_.compose();
    if (!buffer) {
        roc_log(LogError, "rtp composer: can't allocate buffer");
        return NULL;
    }

    RTP_Packet rtp_packet;
    rtp_packet.compose(buffer);

    if (type == packet::IAudioPacket::Type) {
        return new (audio_pool_) AudioPacket(audio_pool_, rtp_packet, NULL);
    }

    if (type == packet::IFECPacket::Type) {
        rtp_packet.header().set_payload_type(123); // FIXME
        return new (fec_pool_) FECPacket(fec_pool_, rtp_packet);
    }

    roc_log(LogError, "rtp composer: bad packet type");
    return NULL;
}

} // namespace rtp
} // namespace roc
