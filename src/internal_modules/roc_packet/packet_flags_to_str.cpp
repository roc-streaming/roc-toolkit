/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/packet_flags_to_str.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace packet {

namespace {

const char* flag_to_str(Packet::Flag flag) {
    switch (flag) {
    case Packet::FlagUDP:
        return "udp";
    case Packet::FlagRTP:
        return "rtp";
    case Packet::FlagFEC:
        return "fec";
    case Packet::FlagRTCP:
        return "rtcp";
    case Packet::FlagAudio:
        return "audio";
    case Packet::FlagControl:
        return "control";
    case Packet::FlagRepair:
        return "repair";
    case Packet::FlagPrepared:
        return "prepared";
    case Packet::FlagComposed:
        return "composed";
    case Packet::FlagRestored:
        return "restored";
    }
    return "?";
}

} // namespace

packet_flags_to_str::packet_flags_to_str(unsigned flags) {
    core::StringBuilder bld(buf_, sizeof(buf_));

    bld.append_str("[");

    bool is_first = true;

    for (unsigned i = 0; i < sizeof(flags) * 8; i++) {
        const Packet::Flag flag = (Packet::Flag)(1 << i);
        if (flags & (unsigned)flag) {
            if (!is_first) {
                bld.append_str(",");
            }
            bld.append_str(flag_to_str(flag));
            is_first = false;
        }
    }

    bld.append_str("]");
}

} // namespace packet
} // namespace roc
