/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/shared_ptr.h"
#include "roc_core/log.h"

#include "roc_rtp/parser.h"
#include "roc_rtp/audio_format.h"

namespace roc {
namespace rtp {

Parser::Parser(core::IPool<AudioPacket>& audio_pool,
               core::IPool<ContainerPacket>& container_pool)
    : audio_pool_(audio_pool)
    , container_pool_(container_pool) {
}

packet::IPacketConstPtr Parser::parse(const core::IByteBufferConstSlice& buffer) {
    if (!buffer) {
        roc_panic("rtp parser: null buffer");
    }

    if (buffer.size() < sizeof(RTP_Header)) {
        roc_log(LogDebug, "rtp parser: bad packet, size < %d (rtp preamble)",
                (int)sizeof(RTP_Header));
        return NULL;
    }

    const RTP_Header& header = *(const RTP_Header*)buffer.data();

    if (header.version() != RTP_V2) {
        roc_log(LogDebug, "rtp parser: bad version, get %d, expected %d",
                (int)header.version(), (int)RTP_V2);
        return NULL;
    }

    size_t header_size = header.header_size();

    if (header.has_extension()) {
        header_size += sizeof(RTP_ExtentionHeader);
    }

    if (buffer.size() < header_size) {
        roc_log(LogDebug, "rtp parser: bad packet, size < %d (rtp header + ext header)",
                (int)header_size);
        return NULL;
    }

    if (header.has_extension()) {
        const RTP_ExtentionHeader& extension =
            *(const RTP_ExtentionHeader*)(buffer.data() + header.header_size());

        header_size += extension.data_size();
    }

    if (buffer.size() < header_size) {
        roc_log(LogDebug,
                "rtp parser: bad packet, size < %d (rtp header + ext header + ext data)",
                (int)header_size);
        return NULL;
    }

    const uint8_t* payload_begin = buffer.data() + header_size;
    const uint8_t* payload_end = buffer.data() + buffer.size();

    if (header.has_padding()) {
        if (payload_begin == payload_end) {
            roc_log(LogDebug,
                    "rtp parser: bad packet, empty payload but padding flag is set");
            return NULL;
        }

        const uint8_t pad_size = payload_end[-1];

        if (pad_size == 0) {
            roc_log(LogDebug, "rtp parser: bad packet, padding size octet is zero");
            return NULL;
        }

        if (size_t(payload_end - payload_begin) < size_t(pad_size)) {
            roc_log(LogDebug,
                    "rtp parser: bad packet, padding size octet > %d (payload size)",
                    (int)(payload_end - payload_begin));
            return NULL;
        }

        payload_end -= pad_size;
    }

    const size_t payload_offset = size_t(payload_begin - buffer.data());
    const size_t payload_size = size_t(payload_end - payload_begin);

    const uint8_t pt = header.payload_type();

    core::SharedPtr<Packet> packet;

    if (const AudioFormat* format = get_audio_format_pt(pt)) {
        packet = new (audio_pool_) AudioPacket(audio_pool_, format);
    }

    if (pt == 123) { // FIXME
        packet = new (container_pool_) ContainerPacket(container_pool_);
    }

    if (!packet) {
        roc_log(LogDebug, "rtp parser: bad payload type %u", (unsigned)pt);
        return NULL;
    }

    packet->parse(buffer, payload_offset, payload_size);

    return packet;
}

} // namespace rtp
} // namespace roc
