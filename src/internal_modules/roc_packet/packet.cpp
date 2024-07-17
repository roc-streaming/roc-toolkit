/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/packet.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace packet {

Packet::Packet(core::IPool& packet_pool)
    : core::RefCounted<Packet, core::PoolAllocation>(packet_pool)
    , flags_(0) {
}

void Packet::add_flags(unsigned flags) {
    if (flags_ & flags) {
        roc_panic("packet: can't add flag more than once");
    }
    flags_ |= flags;
}

bool Packet::has_flags(unsigned flags) const {
    return (flags_ & flags) == flags;
}

unsigned Packet::flags() const {
    return flags_;
}

const UDP* Packet::udp() const {
    if (flags_ & FlagUDP) {
        return &udp_;
    }
    return NULL;
}

UDP* Packet::udp() {
    if (flags_ & FlagUDP) {
        return &udp_;
    }
    return NULL;
}

const RTP* Packet::rtp() const {
    if (flags_ & FlagRTP) {
        return &rtp_;
    }
    return NULL;
}

RTP* Packet::rtp() {
    if (flags_ & FlagRTP) {
        return &rtp_;
    }
    return NULL;
}

const FEC* Packet::fec() const {
    if (flags_ & FlagFEC) {
        return &fec_;
    }
    return NULL;
}

FEC* Packet::fec() {
    if (flags_ & FlagFEC) {
        return &fec_;
    }
    return NULL;
}

const RTCP* Packet::rtcp() const {
    if (flags_ & FlagRTCP) {
        return &rtcp_;
    }
    return NULL;
}

RTCP* Packet::rtcp() {
    if (flags_ & FlagRTCP) {
        return &rtcp_;
    }
    return NULL;
}

const core::Slice<uint8_t>& Packet::buffer() const {
    if (!buffer_) {
        roc_panic("packet: data is null");
    }
    return buffer_;
}

void Packet::set_buffer(const core::Slice<uint8_t>& d) {
    if (buffer_) {
        roc_panic("packet: can't set data more than once");
    }
    buffer_ = d;
}

const core::Slice<uint8_t>& Packet::payload() const {
    if (!buffer_) {
        roc_panic("packet: data is null");
    }

    if (const RTP* r = rtp()) {
        return r->payload;
    }

    if (const RTCP* r = rtcp()) {
        return r->payload;
    }

    if (const FEC* f = fec()) {
        return f->payload;
    }

    return buffer_;
}

bool Packet::has_source_id() const {
    if (rtp()) {
        return true;
    }

    return false;
}

stream_source_t Packet::source_id() const {
    if (const RTP* r = rtp()) {
        return r->source_id;
    }

    return 0;
}

stream_timestamp_t Packet::stream_timestamp() const {
    if (const RTP* r = rtp()) {
        return r->stream_timestamp;
    }

    return 0;
}

stream_timestamp_t Packet::duration() const {
    if (const RTP* r = rtp()) {
        return r->duration;
    }

    return 0;
}

core::nanoseconds_t Packet::capture_timestamp() const {
    if (const RTP* r = rtp()) {
        return r->capture_timestamp;
    }

    return 0;
}

core::nanoseconds_t Packet::receive_timestamp() const {
    if (const UDP* u = udp()) {
        return u->receive_timestamp;
    }

    return 0;
}

int Packet::compare(const Packet& other) const {
    if (const RTP* ra = rtp()) {
        if (const RTP* rb = other.rtp()) {
            return ra->compare(*rb);
        }
    }

    if (const FEC* fa = fec()) {
        if (const FEC* fb = other.fec()) {
            return fa->compare(*fb);
        }
    }

    return 0;
}

size_t Packet::approx_size(size_t n_samples) {
    const size_t approx_header_size = 64;
    const size_t approx_sample_size = 2;

    return approx_header_size + n_samples * approx_sample_size;
}

} // namespace packet
} // namespace roc
