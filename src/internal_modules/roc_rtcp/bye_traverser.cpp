/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/bye_traverser.h"
#include "roc_packet/units.h"
#include "roc_rtcp/headers.h"

#include <string.h>

namespace roc {
namespace rtcp {

ByeTraverser::ByeTraverser(const core::Slice<uint8_t>& buf)
    : buf_(buf)
    , parsed_(false)
    , packet_len_(0)
    , ssrc_count_(0) {
    roc_panic_if_msg(!buf, "bye traverser: null slice");
}

bool ByeTraverser::parse() {
    roc_panic_if_msg(parsed_, "bye traverser: packet already parsed");

    if (buf_.size() < sizeof(header::ByePacket)) {
        return false;
    }

    const header::ByePacket* bye = (const header::ByePacket*)buf_.data();
    if (bye->header().type() != header::RTCP_BYE) {
        return false;
    }

    packet_len_ = bye->header().len_bytes();
    if (packet_len_ > buf_.size()) {
        packet_len_ = 0;
        return false;
    }

    ssrc_count_ = bye->header().counter();
    if (ssrc_count_ > header::PacketMaxBlocks) {
        packet_len_ = 0;
        ssrc_count_ = 0;
        return false;
    }

    parsed_ = true;
    return true;
}

ByeTraverser::Iterator ByeTraverser::iter() const {
    roc_panic_if_msg(!parsed_, "bye traverser: packet not parsed");

    Iterator iter(*this);
    return iter;
}

size_t ByeTraverser::ssrc_count() const {
    roc_panic_if_msg(!parsed_, "bye traverser: packet not parsed");

    return ssrc_count_;
}

ByeTraverser::Iterator::Iterator(const ByeTraverser& traverser)
    : traverser_(traverser)
    , state_(BEGIN)
    , buf_(traverser.buf_)
    , cur_pos_(0)
    , cur_ssrc_(0)
    , error_(false)
    , parsed_ssrc_(0) {
    parsed_reason_[0] = '\0';
}

ByeTraverser::Iterator::State ByeTraverser::Iterator::next() {
    next_element_();
    return state_;
}

bool ByeTraverser::Iterator::error() const {
    return error_;
}

void ByeTraverser::Iterator::next_element_() {
    if (state_ == END) {
        return;
    }

    if (state_ == BEGIN) {
        // Skip packet header.
        cur_pos_ += sizeof(header::ByePacket);
        if (cur_pos_ > buf_.size()) {
            // Packet header larger than buffer.
            error_ = true;
            state_ = END;
            return;
        }
    }

    if (state_ == BEGIN || state_ == SSRC) {
        if (state_ == SSRC) {
            // Go to next SSRC.
            cur_ssrc_++;
            cur_pos_ += sizeof(header::ByeSourceHeader);
        }

        if (cur_ssrc_ == traverser_.ssrc_count_) {
            // No more SSRCs.
            if (cur_pos_ < buf_.size()) {
                // There is also REASON.
                if (cur_pos_ + sizeof(header::ByeReasonHeader) > buf_.size()) {
                    // REASON header larger than remaining buffer.
                    error_ = true;
                    state_ = END;
                    return;
                }

                const header::ByeReasonHeader* reason =
                    (const header::ByeReasonHeader*)&buf_[cur_pos_];
                if (cur_pos_ + sizeof(header::ByeReasonHeader) + reason->text_len()
                    > buf_.size()) {
                    // REASON text larger than remaining buffer.
                    error_ = true;
                    state_ = END;
                    return;
                }

                state_ = REASON;
                parse_reason_();
            } else {
                state_ = END;
            }
        } else {
            // One more SSRC.
            if (cur_pos_ + sizeof(header::ByeSourceHeader) > buf_.size()) {
                // SSRC header larger than remaining buffer.
                error_ = true;
                state_ = END;
                return;
            }
            state_ = SSRC;
            parse_ssrc_();
        }
    } else if (state_ == REASON) {
        // Last element.
        state_ = END;
    } else {
        roc_panic("bye traverser: impossible state");
    }
}

packet::stream_source_t ByeTraverser::Iterator::get_ssrc() const {
    roc_panic_if_msg(state_ != SSRC, "bye traverser: get_ssrc() called in wrong state %d",
                     (int)state_);

    return parsed_ssrc_;
}

const char* ByeTraverser::Iterator::get_reason() const {
    roc_panic_if_msg(state_ != REASON,
                     "bye traverser: get_reason() called in wrong state %d", (int)state_);

    return parsed_reason_;
}

void ByeTraverser::Iterator::parse_ssrc_() {
    const header::ByeSourceHeader* hdr = (const header::ByeSourceHeader*)&buf_[cur_pos_];

    parsed_ssrc_ = hdr->ssrc();
}

void ByeTraverser::Iterator::parse_reason_() {
    const header::ByeReasonHeader* hdr = (const header::ByeReasonHeader*)&buf_[cur_pos_];

    size_t text_len = hdr->text_len();

    text_len = std::min(text_len, size_t(buf_.data_end() - hdr->text()));
    text_len = std::min(text_len, sizeof(parsed_reason_) - 1);

    if (text_len) {
        memcpy(parsed_reason_, hdr->text(), text_len);
    }
    parsed_reason_[text_len] = '\0';
}

} // namespace rtcp
} // namespace roc
