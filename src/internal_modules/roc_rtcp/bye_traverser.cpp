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

ByeTraverser::ByeTraverser(const core::Slice<uint8_t>& data)
    : data_(data)
    , parsed_(false)
    , packet_len_(0)
    , ssrc_count_(0) {
    roc_panic_if_msg(!data, "bye traverser: slice is null");
}

bool ByeTraverser::parse() {
    parsed_ = false;

    if (data_.size() < sizeof(header::ByePacket)) {
        return false;
    }

    header::ByePacket* bye = (header::ByePacket*)data_.data();
    if (bye->header().type() != header::RTCP_BYE) {
        return false;
    }

    packet_len_ = bye->header().len_bytes();
    if (packet_len_ > data_.size()) {
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
    roc_panic_if_msg(!parsed_,
                     "bye traverser:"
                     " iter() called before parse() or parse() returned false");

    Iterator iter(*this);
    return iter;
}

size_t ByeTraverser::ssrc_count() const {
    roc_panic_if_msg(!parsed_,
                     "bye traverser:"
                     " ssrc_count() called before parse() or parse() returned false");

    return ssrc_count_;
}

ByeTraverser::Iterator::Iterator(const ByeTraverser& traverser)
    : traverser_(traverser)
    , state_(BEGIN)
    , data_(traverser.data_)
    , pcur_(traverser.data_.data() + sizeof(header::ByePacket))
    , cur_ssrc_(0)
    , parsed_ssrc_(0) {
    parsed_reason_[0] = '\0';
}

ByeTraverser::Iterator::State ByeTraverser::Iterator::next() {
    roc_panic_if(state_ != BEGIN && state_ != SSRC && state_ != REASON && state_ != END);

    if (state_ == BEGIN || state_ == SSRC) {
        if (state_ == SSRC) {
            roc_panic_if_msg(cur_ssrc_ >= traverser_.ssrc_count_,
                             "bye traverser: element counter outside of bounds");

            cur_ssrc_++;
            pcur_ += sizeof(header::ByeSourceHeader);
        }

        // No more SSRCs.
        if (cur_ssrc_ == traverser_.ssrc_count_) {
            // There is Reason string.
            if (*pcur_ && (pcur_ + *pcur_) < data_.data_end()) {
                state_ = REASON;
                parse_reason_();
            } else {
                state_ = END;
            }
        } else {
            state_ = SSRC;
            parse_ssrc_();
        }
    } else if (state_ == REASON) {
        state_ = END;
    }

    return state_;
}

packet::stream_source_t ByeTraverser::Iterator::ssrc() const {
    roc_panic_if_msg(
        state_ != SSRC,
        "bye traverser:"
        " attempt to access ssrc in bye packet with uninitialized or finished iterator");

    return parsed_ssrc_;
}

const char* ByeTraverser::Iterator::reason() const {
    roc_panic_if_msg(state_ != REASON,
                     "bye traverser:"
                     " attempt to access reason string, but the iterator in other state");

    return parsed_reason_;
}

void ByeTraverser::Iterator::parse_ssrc_() {
    header::ByeSourceHeader* p = (header::ByeSourceHeader*)pcur_;

    parsed_ssrc_ = p->ssrc();
}

void ByeTraverser::Iterator::parse_reason_() {
    header::ByeReasonHeader* p = (header::ByeReasonHeader*)pcur_;

    size_t text_len = p->text_len();

    text_len = std::min(text_len, size_t(data_.data_end() - p->text()));
    text_len = std::min(text_len, sizeof(parsed_reason_) - 1);

    if (text_len) {
        memcpy(parsed_reason_, p->text(), text_len);
    }
    parsed_reason_[text_len] = '\0';
}

} // namespace rtcp
} // namespace roc
