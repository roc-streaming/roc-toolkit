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

SdesTraverser::SdesTraverser(const core::Slice<uint8_t>& buf)
    : buf_(buf)
    , parsed_(false)
    , chunks_count_(0) {
    roc_panic_if_msg(!buf, "sdes traverser: null slice");
}

bool SdesTraverser::parse() {
    roc_panic_if_msg(parsed_, "sdes traverser: packet already parsed");

    if (buf_.size() < sizeof(header::SdesPacket)) {
        return false;
    }

    const header::SdesPacket* sdes = (const header::SdesPacket*)buf_.data();
    if (sdes->header().type() != header::RTCP_SDES) {
        return false;
    }

    const size_t packet_len = sdes->header().len_bytes();
    if (packet_len > buf_.size()) {
        return false;
    }

    chunks_count_ = sdes->header().counter();
    if (chunks_count_ > header::MaxPacketBlocks) {
        return false;
    }

    // Remove padding.
    if (sdes->header().has_padding()) {
        const uint8_t padding_len = buf_[packet_len - 1];
        if (padding_len < 1 || padding_len > packet_len - sizeof(header::SdesPacket)) {
            return false;
        }
        buf_ = buf_.subslice(0, packet_len - padding_len);
    }

    parsed_ = true;
    return true;
}

SdesTraverser::Iterator SdesTraverser::iter() const {
    roc_panic_if_msg(!parsed_, "sdes traverser: packet not parsed");

    Iterator iterator(*this);
    return iterator;
}

size_t SdesTraverser::chunks_count() const {
    roc_panic_if_msg(!parsed_, "sdes traverser: packet not parsed");

    return chunks_count_;
}

SdesTraverser::Iterator::Iterator(const SdesTraverser& traverser)
    : traverser_(traverser)
    , state_(BEGIN)
    , buf_(traverser.buf_)
    , cur_pos_(0)
    , cur_chunk_(0)
    , cur_item_header_(NULL)
    , cur_item_len_(0)
    , error_(false)
    , parsed_ssrc_(0)
    , parsed_item_type_() {
    parsed_item_text_[0] = '\0';
}

SdesTraverser::Iterator::State SdesTraverser::Iterator::next() {
    next_element_();
    return state_;
}

bool SdesTraverser::Iterator::error() const {
    return error_;
}

void SdesTraverser::Iterator::next_element_() {
    if (state_ == END) {
        return;
    }

    if (state_ == BEGIN) {
        // Skip packet header.
        cur_pos_ += sizeof(header::PacketHeader);
        if (cur_pos_ > buf_.size()) {
            // Packet header larger than buffer.
            error_ = true;
            state_ = END;
            return;
        }
    } else {
        if (state_ == CHUNK) {
            // Go to first item after chunk header.
            cur_pos_ += sizeof(header::SdesChunkHeader);
            state_ = ITEM;
        } else if (state_ == ITEM) {
            // Go to next item.
            cur_pos_ += cur_item_len_;
            state_ = ITEM;
        }
    }

    if (state_ == ITEM) {
        if (cur_pos_ == buf_.size()) {
            // Last item in chunk (no more bytes in buffer).
            state_ = CHUNK;
            cur_chunk_++;
        } else if (buf_[cur_pos_] == 0) {
            // Last item in chunk (item type is zero).
            do {
                // Skip padding to 32-bit boundary.
                cur_pos_++;
            } while (cur_pos_ < buf_.size() && (cur_pos_ & 0x03));
            state_ = CHUNK;
            cur_chunk_++;
        } else {
            // Next item.
            if (cur_pos_ + sizeof(header::SdesItemHeader) > buf_.size()) {
                // Item header larger than remaining buffer.
                error_ = true;
                state_ = END;
                return;
            }

            cur_item_header_ = (const header::SdesItemHeader*)&buf_[cur_pos_];
            cur_item_len_ = sizeof(header::SdesItemHeader) + cur_item_header_->text_len();

            if (cur_pos_ + cur_item_len_ > buf_.size()) {
                // Item length larger than remaining buffer.
                error_ = true;
                state_ = END;
                return;
            }

            parse_item_();
            return;
        }
    }

    if (state_ == BEGIN || state_ == CHUNK) {
        if (cur_chunk_ == traverser_.chunks_count_) {
            // Last chunk.
            state_ = END;
            return;
        }

        if (cur_pos_ + sizeof(header::SdesChunkHeader) > buf_.size()) {
            // Chunk header larger than remaining buffer.
            error_ = true;
            state_ = END;
            return;
        }

        state_ = CHUNK;
        parse_chunk_();
        return;
    }

    roc_panic("sdes traverser: impossible state");
}

SdesChunk SdesTraverser::Iterator::get_chunk() const {
    roc_panic_if_msg(state_ != CHUNK,
                     "sdes traverser: get_chunk() called in wrong state %d", (int)state_);

    SdesChunk chunk;
    chunk.ssrc = parsed_ssrc_;

    return chunk;
}

SdesItem SdesTraverser::Iterator::get_item() const {
    roc_panic_if_msg(state_ != ITEM,
                     "sdes traverser: get_item() called in wrong state %d", (int)state_);

    SdesItem item;
    item.type = parsed_item_type_;
    item.text = parsed_item_text_;

    return item;
}

void SdesTraverser::Iterator::parse_chunk_() {
    const header::SdesChunkHeader* hdr = (const header::SdesChunkHeader*)&buf_[cur_pos_];

    parsed_ssrc_ = hdr->ssrc();
}

void SdesTraverser::Iterator::parse_item_() {
    const header::SdesItemHeader* hdr = (const header::SdesItemHeader*)&buf_[cur_pos_];

    size_t text_len = hdr->text_len();

    text_len = std::min(text_len, size_t(buf_.data_end() - hdr->text()));
    text_len = std::min(text_len, sizeof(parsed_item_text_) - 1);

    if (text_len) {
        memcpy(parsed_item_text_, hdr->text(), text_len);
    }
    parsed_item_text_[text_len] = '\0';
    parsed_item_type_ = hdr->type();
}

} // namespace rtcp
} // namespace roc
