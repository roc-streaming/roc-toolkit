/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_rtp/parser.h"
#include "roc_rtp/audio_format.h"

namespace roc {
namespace rtp {

Parser::Parser(core::IPool<AudioPacket>& audio_pool, core::IPool<FECPacket>& fec_pool)
    : audio_pool_(audio_pool)
    , fec_pool_(fec_pool) {
}

packet::IPacketConstPtr Parser::parse(const core::IByteBufferConstSlice& buffer) {
    RTP_Packet rtp_packet;

    if (!rtp_packet.parse(buffer)) {
        roc_log(LogDebug, "rtp parser: bad packet layout");
        return NULL;
    }

    const uint8_t pt = rtp_packet.header().payload_type();

    if (const AudioFormat* format = get_audio_format_pt(pt)) {
        return new (audio_pool_) AudioPacket(audio_pool_, rtp_packet, format);
    }

    // FIXME
    if (pt == 123) {
        return new (fec_pool_) FECPacket(fec_pool_, rtp_packet);
    }

    roc_log(LogDebug, "rtp parser: bad payload type %u", (unsigned)pt);
    return NULL;
}

} // namespace rtp
} // namespace roc
