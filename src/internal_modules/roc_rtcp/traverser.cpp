/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/traverser.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

namespace {

// Check validness of all packets inside this compound packet.
//
// RFC3550, Page 82:
// o  RTP version field must equal 2.
//
// o  The payload type field of the first RTCP packet in a compound
//    packet must be equal to SR or RR.
//
// o  The padding bit (P) should be zero for the first packet of a
//    compound RTCP packet because padding should only be applied, if it
//    is needed, to the last packet.
//
// o  The length fields of the individual RTCP packets must add up to
//    the overall length of the compound RTCP packet as received.  This
//    is a fairly strong check.
bool validate(const core::Slice<uint8_t>& data) {
    bool res = true;
    for (size_t i = 0; res && i < data.size();) {
        const header::PacketHeader& header = *(header::PacketHeader*)&data[i];
        res = header.version() == header::V2;
        res = res
            && (header.type() == header::RTCP_SDES || header.type() == header::RTCP_SR
                || header.type() == header::RTCP_RR || header.type() == header::RTCP_BYE
                || header.type() == header::RTCP_APP || header.type() == header::RTCP_XR);

        const size_t packet_sz = header.len_bytes();
        if ((i + packet_sz) > data.size()) {
            res = false;
            break;
        } else {
            i += packet_sz;
        }
        // Check alignment.
        res = res && (i & 0x03) == 0;
    }
    return res;
}

} // namespace

Traverser::Traverser(const core::Slice<uint8_t>& buffer)
    : data_(buffer)
    , parsed_(false) {
}

bool Traverser::parse() {
    parsed_ = false;

    if (!validate(data_)) {
        return false;
    }

    parsed_ = true;
    return true;
}

Traverser::Iterator Traverser::iter() {
    roc_panic_if_msg(!parsed_,
                     "traverser:"
                     " iter() called before parse() or parse() returned false");

    Traverser::Iterator res(data_);
    return res;
}

Traverser::Iterator::Iterator(const Traverser& traverser)
    : state_(BEGIN)
    , data_(traverser.data_)
    , cur_pkt_header_(NULL)
    , cur_pkt_len_(0)
    , cur_i_(0) {
}

Traverser::Iterator::State Traverser::Iterator::next() {
    next_packet_();
    return state_;
}

void Traverser::Iterator::next_packet_() {
    if (cur_i_ >= data_.size() || state_ == END) {
        state_ = END;
        return;
    }

    if (state_ != BEGIN) {
        skip_packet_();
    }

    uint8_t* p = (uint8_t*)&data_[cur_i_];
    cur_pkt_header_ = (header::PacketHeader*)p;
    cur_pkt_len_ = cur_pkt_header_->len_bytes();

    if (cur_i_ + cur_pkt_len_ > data_.size()) {
        cur_i_ = data_.size();
        state_ = END;
        return;
    }

    cur_slice_ = data_.subslice(cur_i_, cur_i_ + cur_pkt_len_);

    switch ((uint8_t)cur_pkt_header_->type()) {
    case header::RTCP_SR:
        state_ = SR;
        return;
    case header::RTCP_RR:
        state_ = RR;
        return;
    case header::RTCP_SDES:
        state_ = SDES;
        return;
    case header::RTCP_BYE:
        state_ = BYE;
        return;
    case header::RTCP_XR:
        state_ = XR;
        return;
    // Unknown packet type.
    case header::RTCP_APP:
    default:
        skip_packet_();
        return;
    }
}

void Traverser::Iterator::skip_packet_() {
    roc_panic_if(cur_slice_.data_end() < data_.data());

    cur_i_ = (size_t)(cur_slice_.data_end() - data_.data());
}

const header::SenderReportPacket* Traverser::Iterator::get_sr() const {
    roc_panic_if_not(state_ == SR);
    header::SenderReportPacket* sr = (header::SenderReportPacket*)cur_slice_.data();
    return sr;
}

const header::ReceiverReportPacket* Traverser::Iterator::get_rr() const {
    roc_panic_if_not(state_ == RR);
    header::ReceiverReportPacket* rr = (header::ReceiverReportPacket*)cur_slice_.data();
    return rr;
}

XrTraverser Traverser::Iterator::get_xr() const {
    roc_panic_if_not(state_ == XR);
    XrTraverser xr(cur_slice_);
    return xr;
}

SdesTraverser Traverser::Iterator::get_sdes() {
    roc_panic_if_not(state_ == SDES);
    SdesTraverser sdes(cur_slice_);
    return sdes;
}

ByeTraverser Traverser::Iterator::get_bye() {
    roc_panic_if_not(state_ == BYE);
    ByeTraverser bye(cur_slice_);
    return bye;
}

} // namespace rtcp
} // namespace roc
