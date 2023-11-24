/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/traverser.h"
#include "roc_core/log.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

Traverser::Traverser(const core::Slice<uint8_t>& buf)
    : buf_(buf)
    , parsed_(false) {
    roc_panic_if_msg(!buf, "rtcp traverser: null slice");
}

bool Traverser::parse() {
    roc_panic_if_msg(parsed_, "rtcp traverser: packet already parsed");

    if (buf_.size() < sizeof(header::PacketHeader)) {
        return false;
    }

    parsed_ = true;
    return true;
}

Traverser::Iterator Traverser::iter() const {
    roc_panic_if_msg(!parsed_, "rtcp traverser: packet not parsed");

    Iterator res(*this);
    return res;
}

Traverser::Iterator::Iterator(const Traverser& traverser)
    : state_(BEGIN)
    , buf_(traverser.buf_)
    , cur_pos_(0)
    , cur_pkt_header_(NULL)
    , cur_pkt_len_(0)
    , error_(false) {
}

Traverser::Iterator::State Traverser::Iterator::next() {
    next_packet_();
    return state_;
}

bool Traverser::Iterator::error() const {
    return error_;
}

void Traverser::Iterator::next_packet_() {
    if (state_ == END) {
        return;
    }

    if (state_ != BEGIN) {
        // Go to next packet.
        cur_pos_ += cur_pkt_len_;
    }

    // Skip packets until found known type.
    for (;;) {
        if (cur_pos_ == buf_.size()) {
            // Last packet.
            state_ = END;
            return;
        }

        if (cur_pos_ + sizeof(header::PacketHeader) > buf_.size()) {
            // Packet header larger than remaining buffer.
            error_ = true;
            state_ = END;
            return;
        }

        cur_pkt_header_ = (const header::PacketHeader*)&buf_[cur_pos_];
        cur_pkt_len_ = cur_pkt_header_->len_bytes();

        if (cur_pkt_header_->version() != header::V2) {
            // Packet has unexpected version.
            error_ = true;
            state_ = END;
            return;
        }

        if (cur_pos_ + cur_pkt_len_ > buf_.size()) {
            // Packet length larger than remaining buffer.
            error_ = true;
            state_ = END;
            return;
        }

        cur_pkt_slice_ = buf_.subslice(cur_pos_, cur_pos_ + cur_pkt_len_);

        switch (cur_pkt_header_->type()) {
        case header::RTCP_SR:
            if (!check_sr_()) {
                // Skipping invalid SR packet.
                error_ = true;
                break;
            }
            state_ = SR;
            return;
        case header::RTCP_RR:
            if (!check_rr_()) {
                // Skipping invalid RR packet.
                error_ = true;
                break;
            }
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
        default:
            // Unknown packet type.
            break;
        }

        // Skip to next packet.
        cur_pos_ += cur_pkt_len_;
    }
}

bool Traverser::Iterator::check_sr_() {
    const header::SenderReportPacket* sr =
        (const header::SenderReportPacket*)cur_pkt_slice_.data();

    if (cur_pkt_len_ < sizeof(header::SenderReportPacket)
            + sr->num_blocks() * sizeof(header::ReceptionReportBlock)) {
        return false;
    }

    return true;
}

bool Traverser::Iterator::check_rr_() {
    const header::ReceiverReportPacket* rr =
        (const header::ReceiverReportPacket*)cur_pkt_slice_.data();

    if (cur_pkt_len_ < sizeof(header::ReceiverReportPacket)
            + rr->num_blocks() * sizeof(header::ReceptionReportBlock)) {
        return false;
    }

    return true;
}

const header::SenderReportPacket& Traverser::Iterator::get_sr() const {
    roc_panic_if_msg(state_ != SR, "rtcp traverser: get_sr() called in wrong state %d",
                     (int)state_);

    const header::SenderReportPacket* sr =
        (const header::SenderReportPacket*)cur_pkt_slice_.data();
    return *sr;
}

const header::ReceiverReportPacket& Traverser::Iterator::get_rr() const {
    roc_panic_if_msg(state_ != RR, "rtcp traverser: get_rr() called in wrong state %d",
                     (int)state_);

    const header::ReceiverReportPacket* rr =
        (const header::ReceiverReportPacket*)cur_pkt_slice_.data();
    return *rr;
}

XrTraverser Traverser::Iterator::get_xr() const {
    roc_panic_if_msg(state_ != XR, "rtcp traverser: get_xr() called in wrong state %d",
                     (int)state_);

    XrTraverser xr(cur_pkt_slice_);
    return xr;
}

SdesTraverser Traverser::Iterator::get_sdes() {
    roc_panic_if_msg(state_ != SDES,
                     "rtcp traverser: get_sdes() called in wrong state %d", (int)state_);

    SdesTraverser sdes(cur_pkt_slice_);
    return sdes;
}

ByeTraverser Traverser::Iterator::get_bye() {
    roc_panic_if_msg(state_ != BYE, "rtcp traverser: get_bye() called in wrong state %d",
                     (int)state_);

    ByeTraverser bye(cur_pkt_slice_);
    return bye;
}

} // namespace rtcp
} // namespace roc
