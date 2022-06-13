/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/sdes_traverser.h"
#include "roc_rtcp/bye_traverser.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

SdesTraverser::SdesTraverser(const core::Slice<uint8_t>& data)
    : data_(data)
    , parsed_(false)
    , packet_len_(0)
    , chunks_count_(0) {
    roc_panic_if_msg(!data, "traverser: slice is null");
}

bool SdesTraverser::parse() {
    parsed_ = false;

    if (data_.size() < sizeof(header::SdesPacket)) {
        return false;
    }

    header::SdesPacket* sdes = (header::SdesPacket*)data_.data();
    if (sdes->header().type() != header::RTCP_SDES) {
        return false;
    }

    packet_len_ = sdes->header().len_bytes();
    if (packet_len_ > data_.size()) {
        packet_len_ = 0;
        return false;
    }

    chunks_count_ = sdes->header().counter();
    if (chunks_count_ > header::PacketMaxBlocks) {
        packet_len_ = 0;
        chunks_count_ = 0;
        return false;
    }

    parsed_ = true;
    return true;
}

SdesTraverser::Iterator SdesTraverser::iter() const {
    roc_panic_if_msg(!parsed_,
                     "sdes traverser:"
                     " iter() called before parse() or parse() returned false");

    Iterator iterator(*this);
    return iterator;
}

size_t SdesTraverser::chunks_count() const {
    roc_panic_if_msg(!parsed_,
                     "sdes traverser:"
                     " chunks_count() called before parse() or parse() returned false");

    return chunks_count_;
}

SdesTraverser::Iterator::Iterator(const SdesTraverser& traverser)
    : traverser_(traverser)
    , state_(BEGIN)
    , data_(traverser.data_)
    , pcur_(traverser.data_.data() + sizeof(header::PacketHeader))
    , cur_chunk_(0)
    , parsed_ssrc_(0)
    , parsed_item_type_() {
    parsed_item_text_[0] = '\0';
}

SdesTraverser::Iterator::State SdesTraverser::Iterator::next() {
    if (state_ == END) {
        return state_;
    }

    if (state_ == BEGIN) {
        if (cur_chunk_ == traverser_.chunks_count_) {
            state_ = END;
        } else if ((pcur_ + sizeof(header::SdesChunkHeader)) >= data_.data_end()) {
            state_ = END;
        } else {
            state_ = CHUNK;
            parse_chunk_();
        }
    } else if (state_ == CHUNK || state_ == ITEM) {
        roc_panic_if_not(pcur_);

        if (state_ == CHUNK) {
            pcur_ += sizeof(header::SdesChunkHeader);
        } else if (state_ == ITEM) {
            header::SdesItemHeader* p = (header::SdesItemHeader*)pcur_;
            if ((pcur_ + sizeof(header::SdesItemHeader) + p->text_len())
                >= data_.data_end()) {
                state_ = END;
                pcur_ = data_.data_end();
            } else {
                pcur_ += sizeof(header::SdesItemHeader) + p->text_len();
            }
        }

        // Terminating item in the current chunk.
        if (!*pcur_) {
            // Skip padding.
            while (!*pcur_ && pcur_ < data_.data_end() && (pcur_ - data_.data()) & 0x03) {
                pcur_++;
            }
            if (pcur_ - data_.data() >= (ptrdiff_t)traverser_.packet_len_) {
                state_ = END;
            } else if (((pcur_ - data_.data()) & 0x03) != 0
                       || pcur_ >= data_.data_end()) {
                state_ = END;
            } else {
                cur_chunk_++;
                if (cur_chunk_ == traverser_.chunks_count_) {
                    state_ = END;
                } else if ((pcur_ + sizeof(header::SdesChunkHeader))
                           >= data_.data_end()) {
                    state_ = END;
                } else {
                    state_ = CHUNK;
                    parse_chunk_();
                }
            }
        } else if (pcur_ >= data_.data_end()
                   || (pcur_ - data_.data()) >= (ptrdiff_t)traverser_.packet_len_) {
            state_ = END;
        } else {
            state_ = ITEM;
            parse_item_();
        }
    }
    return state_;
}

SdesChunk SdesTraverser::Iterator::chunk() const {
    roc_panic_if_msg(state_ != CHUNK,
                     "sdes traverser:"
                     " attempt to access getter of iterator in inapropriate state %d",
                     (int)state_);

    SdesChunk chunk;
    chunk.ssrc = parsed_ssrc_;

    return chunk;
}

SdesItem SdesTraverser::Iterator::item() const {
    roc_panic_if_msg(state_ != ITEM,
                     "sdes traverser:"
                     " attempt to access getter of iterator in inapropriate state %d",
                     (int)state_);

    SdesItem item;
    item.type = parsed_item_type_;
    item.text = parsed_item_text_;

    return item;
}

void SdesTraverser::Iterator::parse_chunk_() {
    header::SdesChunkHeader* p = (header::SdesChunkHeader*)pcur_;

    parsed_ssrc_ = p->ssrc();
}

void SdesTraverser::Iterator::parse_item_() {
    header::SdesItemHeader* p = (header::SdesItemHeader*)pcur_;

    size_t text_len = p->text_len();

    text_len = std::min(text_len, size_t(data_.data_end() - p->text()));
    text_len = std::min(text_len, sizeof(parsed_item_text_) - 1);

    if (text_len) {
        memcpy(parsed_item_text_, p->text(), text_len);
    }
    parsed_item_text_[text_len] = '\0';
    parsed_item_type_ = p->type();
}

} // namespace rtcp
} // namespace roc
