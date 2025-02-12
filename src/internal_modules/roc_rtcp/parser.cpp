/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/parser.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtcp {

Parser::Parser(core::IArena& arena)
    : IParser(arena) {
}

status::StatusCode Parser::init_status() const {
    return status::StatusOK;
}

status::StatusCode Parser::parse(packet::Packet& packet,
                                 const core::Slice<uint8_t>& buffer) {
    if (!buffer) {
        roc_panic("rtcp parser: buffer is null");
    }

    packet.add_flags(packet::Packet::FlagControl);
    packet.add_flags(packet::Packet::FlagRTCP);

    packet.rtcp()->payload = buffer;

    return status::StatusOK;
}

} // namespace rtcp
} // namespace roc
