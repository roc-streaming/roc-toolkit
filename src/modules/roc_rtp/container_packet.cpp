/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

#include "roc_rtp/container_packet.h"

namespace roc {
namespace rtp {

ContainerPacket::ContainerPacket(core::IPool<ContainerPacket>& pool)
    : pool_(pool) {
}

void ContainerPacket::free() {
    pool_.destroy(*this);
}

int ContainerPacket::options() const {
    return (Packet::options() | HasFEC); // FIXME
}

const packet::IHeaderFECFrame* ContainerPacket::fec() const {
    return this; // FIXME
}

packet::IHeaderFECFrame* ContainerPacket::fec() {
    return this; // FIXME
}

packet::seqnum_t ContainerPacket::source_blknum() const {
    // FIXME
    return header().timestamp() & 0xffff;
}

void ContainerPacket::set_source_blknum(packet::seqnum_t sn) {
    // FIXME
    uint32_t ts = header().timestamp();
    ts &= 0xffff0000;
    ts |= (sn & 0xffff);
    header().set_timestamp(ts);
}

packet::seqnum_t ContainerPacket::repair_blknum() const {
    // FIXME
    return header().timestamp() >> 16;
}

void ContainerPacket::set_repair_blknum(packet::seqnum_t sn) {
    // FIXME
    uint32_t ts = header().timestamp();
    ts &= 0x0000ffff;
    ts |= uint32_t((sn & 0xffff) << 16);
    header().set_timestamp(ts);
}

} // namespace rtp
} // namespace roc
