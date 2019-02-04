/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/print.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/packet.h"
#include "roc_packet/print.h"

namespace roc {
namespace packet {

void print(const Packet& p, int flags) {
    if (p.udp()) {
        fprintf(stderr, "udp: src=%s dst=%s\n", address_to_str(p.udp()->src_addr).c_str(),
                address_to_str(p.udp()->dst_addr).c_str());
    }

    if (p.rtp()) {
        fprintf(stderr, "rtp: src=%lu m=%d sn=%lu ts=%lu dur=%lu pt=%u payload=%lu\n",
                (unsigned long)p.rtp()->source, (int)p.rtp()->marker,
                (unsigned long)p.rtp()->seqnum, (unsigned long)p.rtp()->timestamp,
                (unsigned long)p.rtp()->duration, (unsigned int)p.rtp()->payload_type,
                (unsigned long)p.rtp()->payload.size());

        if ((flags & PrintPayload) && p.rtp()->payload) {
            core::print_memory(p.rtp()->payload.data(), p.rtp()->payload.size());
        }
    }

    if (p.fec()) {
        fprintf(stderr, "fec: sbn=%lu sblen=%lu payload=%lu\n",
                (unsigned long)p.fec()->source_block_number,
                (unsigned long)p.fec()->source_block_length,
                (unsigned long)p.fec()->payload.size());

        if ((flags & PrintPayload) && p.fec()->payload) {
            core::print_memory(p.fec()->payload.data(), p.fec()->payload.size());
        }
    }
}

} // namespace packet
} // namespace roc
