/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_core/math.h"

#include "roc_rtp/packet.h"

namespace roc {
namespace rtp {

Packet::Packet()
    : payload_off_(0)
    , payload_size_(0) {
}

void Packet::compose(const core::IByteBufferPtr& buffer) {
    roc_panic_if(buffer_);

    if (!buffer) {
        roc_panic("rtp packet: null buffer in compose()");
    }

    buffer->set_size(sizeof(RTP_Header));
    buffer_ = *buffer;

    header().clear();
    header().set_version(RTP_V2);
}

void Packet::parse(const core::IByteBufferConstSlice& buffer,
                   size_t payload_off,
                   size_t payload_size) {
    roc_panic_if(buffer_);

    if (!buffer) {
        roc_panic("rtp packet: null buffer in parse()");
    }

    if (payload_off + payload_size > buffer.size()) {
        roc_panic("rtp packet: invalid payload boundaries in parse()");
    }

    buffer_ = buffer;

    payload_off_ = payload_off;
    payload_size_ = payload_size;
}

int Packet::options() const {
    return (HasOrder | HasRTP);
}

const packet::IHeaderOrdering* Packet::order() const {
    return this;
}

const packet::IHeaderRTP* Packet::rtp() const {
    return this;
}

packet::IHeaderRTP* Packet::rtp() {
    return this;
}

const packet::IHeaderFECFrame* Packet::fec() const {
    return NULL;
}

packet::IHeaderFECFrame* Packet::fec() {
    return NULL;
}

const packet::IPayloadAudio* Packet::audio() const {
    return NULL;
}

packet::IPayloadAudio* Packet::audio() {
    return NULL;
}

core::IByteBufferConstSlice Packet::raw_data() const {
    roc_panic_if_not(buffer_);

    return buffer_;
}

core::IByteBufferConstSlice Packet::payload() const {
    roc_panic_if_not(buffer_);

    if (payload_size_) {
        return core::IByteBufferConstSlice(buffer_, payload_off_, payload_size_);
    } else {
        return core::IByteBufferConstSlice();
    }
}

uint8_t* Packet::get_payload() {
    roc_panic_if_not(buffer_);

    return mut_buffer_().data() + payload_off_;
}

void Packet::set_payload(const uint8_t* data, size_t size) {
    if (!data && size) {
        roc_panic("rtp fec packet: data is null, size is non-null");
    }

    resize_payload(size);

    if (size > 0) {
        memcpy(get_payload(), data, size);
    }
}

void Packet::resize_payload(size_t size) {
    mut_buffer_().set_size(sizeof(RTP_Header) + size);
    buffer_ = mut_buffer_();

    payload_off_ = sizeof(RTP_Header);
    payload_size_ = size;
}

const RTP_Header& Packet::header() const {
    roc_panic_if_not(buffer_);

    return *(const RTP_Header*)buffer_.data();
}

RTP_Header& Packet::header() {
    roc_panic_if_not(buffer_);

    return *(RTP_Header*)mut_buffer_().data();
}

const RTP_ExtentionHeader* Packet::extension() const {
    roc_panic_if_not(buffer_);

    if (header().has_extension()) {
        return (const RTP_ExtentionHeader*)(buffer_.data() + header().header_size());
    } else {
        return NULL;
    }
}

bool Packet::is_same_flow(const packet::IPacket& other) const {
    roc_panic_if_not(other.rtp());

    return source() == other.rtp()->source();
}

bool Packet::is_before(const packet::IPacket& other) const {
    roc_panic_if_not(other.rtp());

    return ROC_IS_BEFORE(packet::signed_seqnum_t, seqnum(), other.rtp()->seqnum());
}

packet::source_t Packet::source() const {
    return header().ssrc();
}

void Packet::set_source(packet::source_t s) {
    header().set_ssrc(s);
}

packet::seqnum_t Packet::seqnum() const {
    return header().seqnum();
}

void Packet::set_seqnum(packet::seqnum_t sn) {
    header().set_seqnum(sn);
}

packet::timestamp_t Packet::timestamp() const {
    return header().timestamp();
}

void Packet::set_timestamp(packet::timestamp_t ts) {
    header().set_timestamp(ts);
}

size_t Packet::rate() const {
    return 0;
}

bool Packet::marker() const {
    return header().marker();
}

void Packet::set_marker(bool m) {
    header().set_marker(m);
}

core::IByteBuffer& Packet::mut_buffer_() {
    roc_panic_if_not(buffer_);
    roc_panic_if_not(buffer_.data() == buffer_.container()->data());

    return *const_cast<core::IByteBuffer*>(buffer_.container().get());
}

} // namespace rtp
} // namespace roc
