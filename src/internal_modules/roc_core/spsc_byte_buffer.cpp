/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/spsc_byte_buffer.h"
#include "roc_core/atomic_ops.h"

namespace roc {
namespace core {

SpscByteBuffer::SpscByteBuffer(IArena& arena, size_t chunk_size, size_t n_chunks)
    : arena_(arena)
    , chunk_size_(chunk_size)
    , chunk_count_(n_chunks + 1) // +1 guard chunk
    , read_pos_(0)
    , write_pos_(1) {
    memory_ =
        arena_.allocate(sizeof(uint8_t*) * chunk_count_ + chunk_size_ * chunk_count_);
    if (!memory_) {
        return;
    }

    chunks_ = (uint8_t**)memory_;

    for (size_t n = 0; n < chunk_count_; n++) {
        chunks_[n] =
            ((uint8_t*)memory_) + (sizeof(uint8_t*) * chunk_count_) + (chunk_size_ * n);
    }
}

SpscByteBuffer::~SpscByteBuffer() {
    if (memory_) {
        arena_.deallocate(memory_);
    }
}

bool SpscByteBuffer::is_valid() const {
    return memory_ != NULL;
}

bool SpscByteBuffer::is_empty() const {
    roc_panic_if(!is_valid());

    const uint32_t wr_pos = AtomicOps::load_seq_cst(write_pos_);
    const uint32_t rd_pos = AtomicOps::load_seq_cst(read_pos_);

    return (rd_pos + 1 == wr_pos);
}

uint8_t* SpscByteBuffer::begin_write() {
    roc_panic_if(!is_valid());

    const uint32_t wr_pos = AtomicOps::load_relaxed(write_pos_);
    const uint32_t rd_pos = AtomicOps::load_seq_cst(read_pos_);

    if (wr_pos - rd_pos >= chunk_count_) {
        return NULL;
    }

    return chunks_[wr_pos % chunk_count_];
}

void SpscByteBuffer::end_write() {
    roc_panic_if(!is_valid());

    AtomicOps::fetch_add_seq_cst(write_pos_, 1u);
}

uint8_t* SpscByteBuffer::begin_read() {
    roc_panic_if(!is_valid());

    const uint32_t rd_pos = AtomicOps::load_relaxed(read_pos_);
    const uint32_t wr_pos = AtomicOps::load_seq_cst(write_pos_);

    if (rd_pos + 1 == wr_pos) {
        return NULL;
    }

    return chunks_[(rd_pos + 1) % chunk_count_];
}

void SpscByteBuffer::end_read() {
    roc_panic_if(!is_valid());

    AtomicOps::fetch_add_seq_cst(read_pos_, 1u);
}

} // namespace core
} // namespace roc
