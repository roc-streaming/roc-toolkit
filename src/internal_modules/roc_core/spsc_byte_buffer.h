/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/spsc_byte_buffer.h
//! @brief Single-producer single-consumer circular buffer of byte chunks.

#ifndef ROC_CORE_SPSC_BYTE_BUFFER_H_
#define ROC_CORE_SPSC_BYTE_BUFFER_H_

#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Thread-safe lock-free single-producer single-consumer
//! circular buffer of byte chunks.
//!
//! Allows access from two concurrent threads: writer and reader.
//! Both writer and reader are never blocked.
//! Provides sequential consistency.
class SpscByteBuffer : public NonCopyable<> {
public:
    //! Initialize.
    SpscByteBuffer(IArena& arena, size_t chunk_size, size_t n_chunks);

    //! Deinitialize.
    ~SpscByteBuffer();

    //! Check that initial allocation succeeded.
    bool is_valid() const;

    //! Check if buffer is empty.
    bool is_empty() const;

    //! Begin writing of a chunk.
    //! If buffer is full, returns NULL.
    //! Should be called from writer thread.
    //! Lock-free.
    uint8_t* begin_write();

    //! End writing of a chunk.
    //! Should be called if and only if begin_write() returned non-NULL.
    //! Should be called from writer thread.
    //! Lock-free.
    void end_write();

    //! Begin reading of a chunk.
    //! If buffer is empty, returns NULL.
    //! Should be called from reader thread.
    //! Lock-free.
    uint8_t* begin_read();

    //! End reading of a chunk.
    //! Should be called if and only if begin_read() returned non-NULL.
    //! Should be called from reader thread.
    //! Lock-free.
    void end_read();

private:
    IArena& arena_;

    const size_t chunk_size_;
    const size_t chunk_count_;

    void* memory_;
    uint8_t** chunks_;

    uint32_t read_pos_;
    uint32_t write_pos_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SPSC_BYTE_BUFFER_H_
