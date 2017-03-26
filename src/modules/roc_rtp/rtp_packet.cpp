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

#include "roc_rtp/rtp_packet.h"

namespace roc {
namespace rtp {

RTP_Packet::RTP_Packet()
    : payload_off_(0)
    , payload_size_(0) {
}

void RTP_Packet::compose(const core::IByteBufferPtr& buffer) {
    roc_panic_if(buffer_);

    if (!buffer) {
        roc_panic("rtp: can't compose packet for null buffer");
    }

    buffer->set_size(sizeof(RTP_Header));
    buffer_ = *buffer;

    header().clear();
    header().set_version(RTP_V2);
}

bool RTP_Packet::parse(const core::IByteBufferConstSlice& buffer) {
    roc_panic_if(buffer_);

    if (!buffer) {
        roc_panic("rtp: can't compose packet for null buffer");
    }

    if (buffer.size() < sizeof(RTP_Header)) {
        roc_log(LogDebug, "rtp: bad packet, size < %d (rtp preamble)",
                (int)sizeof(RTP_Header));
        return false;
    }

    const RTP_Header& hdr = *(const RTP_Header*)buffer.data();

    if (hdr.version() != RTP_V2) {
        roc_log(LogDebug, "rtp: bad version, get %d, expected %d", (int)hdr.version(),
                (int)RTP_V2);
        return false;
    }

    size_t hdr_size = hdr.header_size();

    if (hdr.has_extension()) {
        hdr_size += sizeof(RTP_ExtentionHeader);
    }

    if (buffer.size() < hdr_size) {
        roc_log(LogDebug, "rtp: bad packet, size < %d (rtp header + ext header)",
                (int)hdr_size);
        return false;
    }

    if (hdr.has_extension()) {
        const RTP_ExtentionHeader& ext =
            *(const RTP_ExtentionHeader*)(buffer.data() + hdr.header_size());

        hdr_size += ext.data_size();
    }

    if (buffer.size() < hdr_size) {
        roc_log(LogDebug,
                "rtp: bad packet, size < %d (rtp header + ext header + ext data)",
                (int)hdr_size);
        return false;
    }

    const uint8_t* payload_begin = buffer.data() + hdr_size;
    const uint8_t* payload_end = buffer.data() + buffer.size();

    if (hdr.has_padding()) {
        if (payload_begin == payload_end) {
            roc_log(LogDebug, "rtp: bad packet, empty payload but padding flag is set");
            return false;
        }

        const uint8_t pad_size = payload_end[-1];

        if (pad_size == 0) {
            roc_log(LogDebug, "rtp: bad packet, padding size octet is zero");
            return false;
        }

        if (size_t(payload_end - payload_begin) < size_t(pad_size)) {
            roc_log(LogDebug, "rtp: bad packet, padding size octet > %d (payload size)",
                    (int)(payload_end - payload_begin));
            return false;
        }

        payload_end -= pad_size;
    }

    buffer_ = buffer;

    payload_off_ = size_t(payload_begin - buffer_.data());
    payload_size_ = size_t(payload_end - payload_begin);

    return true;
}

const core::IByteBufferConstSlice& RTP_Packet::raw_data() const {
    roc_panic_if_not(buffer_);

    return buffer_;
}

const RTP_Header& RTP_Packet::header() const {
    roc_panic_if_not(buffer_);

    return *(const RTP_Header*)buffer_.data();
}

RTP_Header& RTP_Packet::header() {
    roc_panic_if_not(buffer_);

    return *(RTP_Header*)mut_buffer_().data();
}

const RTP_ExtentionHeader* RTP_Packet::ext_header() const {
    roc_panic_if_not(buffer_);

    if (header().has_extension()) {
        return (const RTP_ExtentionHeader*)(buffer_.data() + header().header_size());
    } else {
        return NULL;
    }
}

core::IByteBufferConstSlice RTP_Packet::payload() const {
    roc_panic_if_not(buffer_);

    if (payload_size_) {
        return core::IByteBufferConstSlice(buffer_, payload_off_, payload_size_);
    } else {
        return core::IByteBufferConstSlice();
    }
}

core::IByteBufferSlice RTP_Packet::payload() {
    roc_panic_if_not(buffer_);

    if (payload_size_) {
        return core::IByteBufferSlice(mut_buffer_(), payload_off_, payload_size_);
    } else {
        return core::IByteBufferSlice();
    }
}

void RTP_Packet::set_payload_size(size_t size) {
    roc_panic_if_not(buffer_);

    mut_buffer_().set_size(sizeof(RTP_Header) + size);
    buffer_ = mut_buffer_();

    payload_off_ = sizeof(RTP_Header);
    payload_size_ = size;
}

core::IByteBuffer& RTP_Packet::mut_buffer_() {
    roc_panic_if_not(buffer_);
    roc_panic_if_not(buffer_.data() == buffer_.container()->data());

    return *const_cast<core::IByteBuffer*>(buffer_.container().get());
}

} // namespace rtp
} // namespace roc
