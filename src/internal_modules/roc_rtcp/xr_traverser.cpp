/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/xr_traverser.h"

namespace roc {
namespace rtcp {

XrTraverser::XrTraverser(const core::Slice<uint8_t>& data)
    : data_(data)
    , parsed_(false)
    , packet_len_(0)
    , blocks_count_(0) {
    roc_panic_if_msg(!data, "xr traverser: slice is null");
}

bool XrTraverser::parse() {
    parsed_ = false;

    if (data_.size() < sizeof(header::XrPacket)) {
        return false;
    }

    header::XrPacket* xr = (header::XrPacket*)data_.data();
    if (xr->header().type() != header::RTCP_XR) {
        return false;
    }

    packet_len_ = xr->header().len_bytes();
    if (packet_len_ > data_.size()) {
        packet_len_ = 0;
        return false;
    }

    Iterator iter(*this);
    while (iter.next() != Iterator::END) {
        blocks_count_++;
    }

    parsed_ = true;
    return true;
}

XrTraverser::Iterator XrTraverser::iter() const {
    roc_panic_if_msg(!parsed_,
                     "xr traverser:"
                     " iter() called before parse() or parse() returned false");

    Iterator iter(*this);
    return iter;
}

size_t XrTraverser::blocks_count() const {
    roc_panic_if_msg(!parsed_,
                     "xr traverser:"
                     " blocks_count() called before parse() or parse() returned false");

    return blocks_count_;
}

const header::XrPacket& XrTraverser::packet() const {
    roc_panic_if_msg(!parsed_,
                     "xr traverser:"
                     " packet() called before parse() or parse() returned false");

    return *(const header::XrPacket*)data_.data();
}

XrTraverser::Iterator::Iterator(const XrTraverser& traverser)
    : state_(BEGIN)
    , data_(traverser.data_)
    , pcur_(traverser.data_.data()) {
}

XrTraverser::Iterator::State XrTraverser::Iterator::next() {
    if (state_ == END) {
        return state_;
    }

    if (state_ == BEGIN) {
        pcur_ += sizeof(header::XrPacket);
        if (pcur_ >= data_.data_end()) {
            state_ = END;
            return state_;
        }
    }

    header::XrBlockHeader* p_block_header = (header::XrBlockHeader*)pcur_;

    if (state_ != BEGIN) {
        const size_t block_len = p_block_header->len_bytes();
        pcur_ += block_len;
        p_block_header = (header::XrBlockHeader*)pcur_;
    }

    // Walk through all block packets and seek for known types.
    while (p_block_header->block_type() != header::XR_RRTR
           && p_block_header->block_type() != header::XR_DLRR
           && pcur_ < data_.data_end()) {
        const size_t block_len = p_block_header->len_bytes();
        // If the block is incorrect, skip the whole packet.
        if (pcur_ + block_len > data_.data_end()) {
            state_ = END;
            return state_;
        } else {
            pcur_ += block_len;
            p_block_header = (header::XrBlockHeader*)pcur_;
        }
    }

    // No known block type in XR packet.
    if (pcur_ >= data_.data_end()) {
        state_ = END;
    } else if (p_block_header->block_type() == header::XR_RRTR) {
        state_ = RRTR_BLOCK;
    } else if (p_block_header->block_type() == header::XR_DLRR) {
        state_ = DRLL_BLOCK;
    } else {
        roc_panic("xr traverser: impossible branch");
    }

    return state_;
}

const header::XrRrtrBlock& XrTraverser::Iterator::get_rrtr() const {
    roc_panic_if_msg(state_ != RRTR_BLOCK,
                     "xt traverser:"
                     " attempt to access block with wrong type or at wrong state");
    return *(header::XrRrtrBlock*)pcur_;
}

const header::XrDlrrBlock& XrTraverser::Iterator::get_dlrr() const {
    roc_panic_if_msg(state_ != DRLL_BLOCK,
                     "xt traverser:"
                     " attempt to access block with wrong type or at wrong state");
    return *(header::XrDlrrBlock*)pcur_;
}

} // namespace rtcp
} // namespace roc
