/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/packet.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace packet {

Packet::Packet(PacketPool& pool)
    : pool_(pool)
    , flags_(0) {
}

void Packet::add_flags(unsigned fl) {
    if (flags_ & fl) {
        roc_panic("packet: can't add flag more than once");
    }
    flags_ |= fl;
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

const core::Slice<uint8_t>& Packet::data() const {
    if (!data_) {
        roc_panic("packet: data is null");
    }
    return data_;
}

void Packet::set_data(const core::Slice<uint8_t>& d) {
    if (data_) {
        roc_panic("packet: can't set data more than once");
    }
    data_ = d;
}

source_t Packet::source() const {
    if (const RTP* r = rtp()) {
        return r->source;
    }

    return 0;
}

timestamp_t Packet::begin() const {
    if (const RTP* r = rtp()) {
        return r->timestamp;
    }

    return 0;
}

timestamp_t Packet::end() const {
    if (const RTP* r = rtp()) {
        return r->timestamp + r->duration;
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

void Packet::destroy() {
    pool_.destroy(*this);
}

} // namespace packet
} // namespace roc
