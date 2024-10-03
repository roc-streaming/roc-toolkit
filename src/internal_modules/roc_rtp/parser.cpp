/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/parser.h"
#include "roc_core/log.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

Parser::Parser(packet::IParser* inner_parser,
               const EncodingMap& encoding_map,
               core::IArena& arena)
    : IParser(arena)
    , encoding_map_(encoding_map)
    , inner_parser_(inner_parser) {
}

status::StatusCode Parser::init_status() const {
    return status::StatusOK;
}

status::StatusCode Parser::parse(packet::Packet& packet,
                                 const core::Slice<uint8_t>& buffer) {
    if (buffer.size() < sizeof(Header)) {
        roc_log(LogDebug, "rtp parser: bad packet: size<%d (rtp header)",
                (int)sizeof(Header));
        return status::StatusBadBuffer;
    }

    const Header& header = *(const Header*)buffer.data();

    if (header.version() != V2) {
        roc_log(LogDebug, "rtp parser: bad version: get=%d expected=%d",
                (int)header.version(), (int)V2);
        return status::StatusBadPacket;
    }

    size_t header_size = header.header_size();

    if (header.has_extension()) {
        header_size += sizeof(ExtentionHeader);
    }

    if (buffer.size() < header_size) {
        roc_log(LogDebug, "rtp parser: bad packet: size<%d (rtp header + ext header)",
                (int)header_size);
        return status::StatusBadBuffer;
    }

    if (header.has_extension()) {
        const ExtentionHeader& extension =
            *(const ExtentionHeader*)(buffer.data() + header.header_size());

        header_size += extension.data_size();
    }

    if (buffer.size() < header_size) {
        roc_log(LogDebug,
                "rtp parser: bad packet: size<%d (rtp header + ext header + ext data)",
                (int)header_size);
        return status::StatusBadBuffer;
    }

    size_t payload_begin = header_size;
    size_t payload_end = buffer.size();

    uint8_t pad_size = 0;

    if (header.has_padding()) {
        if (payload_begin == payload_end) {
            roc_log(LogDebug,
                    "rtp parser: bad packet: empty payload but padding flag is set");
            return status::StatusBadPacket;
        }

        pad_size = buffer.data()[payload_end - 1];

        if (pad_size == 0) {
            roc_log(LogDebug, "rtp parser: bad packet: padding size octet is zero");
            return status::StatusBadPacket;
        }

        if (size_t(payload_end - payload_begin) < size_t(pad_size)) {
            roc_log(LogDebug, "rtp parser: bad packet: padding_size>%d (payload size)",
                    (int)(payload_end - payload_begin));
            return status::StatusBadPacket;
        }

        payload_end -= pad_size;
    }

    packet.add_flags(packet::Packet::FlagRTP);

    packet::RTP& rtp = *packet.rtp();

    rtp.source_id = header.ssrc();
    rtp.seqnum = header.seqnum();
    rtp.stream_timestamp = header.timestamp();
    rtp.marker = header.marker();
    rtp.payload_type = header.payload_type();
    rtp.header = buffer.subslice(0, header_size);
    rtp.payload = buffer.subslice(payload_begin, payload_end);

    if (pad_size) {
        rtp.padding = buffer.subslice(payload_end, payload_end + pad_size);
    }

    if (const Encoding* encoding = encoding_map_.find_by_pt(header.payload_type())) {
        packet.add_flags(encoding->packet_flags);
    }

    if (inner_parser_) {
        return inner_parser_->parse(packet, rtp.payload);
    }

    return status::StatusOK;
}

} // namespace rtp
} // namespace roc
