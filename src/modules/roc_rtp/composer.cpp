/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/composer.h"
#include "roc_core/alignment.h"
#include "roc_core/log.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

Composer::Composer(packet::IComposer* inner_composer)
    : inner_composer_(inner_composer) {
}

bool Composer::align(core::Slice<uint8_t>& buffer,
                     size_t header_size,
                     size_t payload_alignment) {
    header_size += sizeof(Header);

    if (inner_composer_ == NULL) {
        const size_t padding = core::padding(header_size, payload_alignment);

        if (buffer.capacity() < padding) {
            roc_log(LogDebug,
                    "rtp composer: not enough space for alignment: padding=%lu cap=%lu",
                    (unsigned long)padding, (unsigned long)buffer.capacity());
            return false;
        }

        buffer.resize(padding);
        buffer = buffer.range(buffer.size(), buffer.size());

        return true;
    } else {
        return inner_composer_->align(buffer, header_size, payload_alignment);
    }
}

bool Composer::prepare(packet::Packet& packet,
                       core::Slice<uint8_t>& buffer,
                       size_t payload_size) {
    core::Slice<uint8_t> header = buffer.range(0, 0);

    if (header.capacity() < sizeof(Header)) {
        roc_log(LogDebug,
                "rtp composer: not enough space for rtp header: size=%lu cap=%lu",
                (unsigned long)sizeof(Header), (unsigned long)header.capacity());
        return false;
    }
    header.resize(sizeof(Header));

    core::Slice<uint8_t> payload = header.range(header.size(), header.size());

    if (inner_composer_ == NULL) {
        if (payload.capacity() < payload_size) {
            roc_log(LogDebug,
                    "rtp composer: not enough space for rtp payload: size=%lu cap=%lu",
                    (unsigned long)payload_size, (unsigned long)payload.capacity());
            return false;
        }
        payload.resize(payload_size);
    } else {
        if (!inner_composer_->prepare(packet, payload, payload_size)) {
            return false;
        }
    }

    packet.add_flags(packet::Packet::FlagRTP);

    packet::RTP& rtp = *packet.rtp();

    rtp.header = header;
    rtp.payload = payload;

    buffer.resize(header.size() + payload.size());

    return true;
}

bool Composer::compose(packet::Packet& packet) {
    if (!packet.rtp()) {
        roc_panic("rtp composer: unexpected non-rtp packet");
    }

    if (packet.rtp()->header.size() != sizeof(Header)) {
        roc_panic("rtp composer: unexpected rtp header size");
    }

    packet::RTP& rtp = *packet.rtp();

    Header& header = *(Header*)rtp.header.data();

    header.clear();
    header.set_version(V2);
    header.set_ssrc(rtp.source);
    header.set_seqnum(rtp.seqnum);
    header.set_timestamp(rtp.timestamp);
    header.set_marker(rtp.marker);
    header.set_payload_type(PayloadType(rtp.payload_type));

    if (inner_composer_) {
        return inner_composer_->compose(packet);
    }

    return true;
}

} // namespace rtp
} // namespace roc
