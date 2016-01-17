/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/stddefs.h"
#include "roc_core/panic.h"

#include "roc_rtp/fec_packet.h"

namespace roc {
namespace rtp {

FECPacket::FECPacket(core::IPool<FECPacket>& pool, const RTP_Packet& packet)
    : packet_(packet)
    , pool_(pool) {
}

void FECPacket::free() {
    pool_.destroy(*this);
}

packet::source_t FECPacket::source() const {
    return packet_.header().ssrc();
}

void FECPacket::set_source(packet::source_t s) {
    packet_.header().set_ssrc(s);
}

packet::seqnum_t FECPacket::seqnum() const {
    return packet_.header().seqnum();
}

void FECPacket::set_seqnum(packet::seqnum_t sn) {
    packet_.header().set_seqnum(sn);
}

packet::timestamp_t FECPacket::timestamp() const {
    // FIXME
    return 0;
}

void FECPacket::set_timestamp(packet::timestamp_t) {
    // FIXME
}

size_t FECPacket::rate() const {
    return 0;
}

bool FECPacket::marker() const {
    return packet_.header().seqnum();
}

void FECPacket::set_marker(bool m) {
    packet_.header().set_marker(m);
}

packet::seqnum_t FECPacket::data_blknum() const {
    // FIXME
    return packet_.header().timestamp() & 0xffff;
}

void FECPacket::set_data_blknum(packet::seqnum_t sn) {
    // FIXME
    uint32_t ts = packet_.header().timestamp();
    ts &= 0xffff0000;
    ts |= (sn & 0xffff);
    packet_.header().set_timestamp(ts);
}

packet::seqnum_t FECPacket::fec_blknum() const {
    // FIXME
    return packet_.header().timestamp() >> 16;
}

void FECPacket::set_fec_blknum(packet::seqnum_t sn) {
    // FIXME
    uint32_t ts = packet_.header().timestamp();
    ts &= 0x0000ffff;
    ts |= uint32_t((sn & 0xffff) << 16);
    packet_.header().set_timestamp(ts);
}

core::IByteBufferConstSlice FECPacket::payload() const {
    return packet_.payload();
}

void FECPacket::set_payload(const uint8_t* data, size_t size) {
    if (!data && size) {
        roc_panic("rtp fec packet: data is null, size is non-null");
    }

    packet_.set_payload_size(size);

    if (size > 0) {
        memcpy(packet_.payload().data(), data, size);
    }
}

core::IByteBufferConstSlice FECPacket::raw_data() const {
    return packet_.raw_data();
}

} // namespace rtp
} // namespace roc
