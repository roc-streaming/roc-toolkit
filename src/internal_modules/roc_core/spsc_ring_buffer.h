/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/spsc_ring_buffer.h
//! @brief Single-producer single-consumer circular buffer of copyable objects.

#ifndef ROC_CORE_SPSC_RING_BUFFER_H_
#define ROC_CORE_SPSC_RING_BUFFER_H_

#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/spsc_byte_buffer.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Thread-safe lock-free single-producer single-consumer
//! circular buffer of copyable objects.
//!
//! Allows access from two concurrent threads: writer and reader.
//! Both writer and reader are never blocked.
//! Provides sequential consistency.
//!
//! @tparam T defines object type, it should be copyable.
//!
//! Implemented on top of SpscByteBuffer.
template <class T> class SpscRingBuffer : public NonCopyable<> {
public:
    //! Initialize.
    SpscRingBuffer(IArena& arena, size_t n_elements)
        : byte_buf_(arena, sizeof(T), n_elements) {
    }

    //! Deinitialize.
    ~SpscRingBuffer() {
        while (void* ptr = byte_buf_.begin_read()) {
            static_cast<T*>(ptr)->~T();
            byte_buf_.end_read();
        }
    }

    //! Check that allocation succeeded.
    bool is_valid() const {
        return byte_buf_.is_valid();
    }

    //! Check if buffer is empty.
    bool is_empty() const {
        return byte_buf_.is_empty();
    }

    //! Append element to the end of the buffer.
    //! If buffer is full, drops element and returns false.
    //! Should be called from writer thread.
    //! Lock-free.
    bool push_back(const T& element) {
        void* ptr = byte_buf_.begin_write();
        if (!ptr) {
            return false;
        }

        new (ptr) T(element);

        byte_buf_.end_write();

        return true;
    }

    //! Fetch element from the beginning of the buffer.
    //! If buffer is empty, returns false.
    //! Should be called from reader thread.
    //! Lock-free.
    bool pop_front(T& element) {
        void* ptr = byte_buf_.begin_read();
        if (!ptr) {
            element = T();
            return false;
        }

        element = *static_cast<T*>(ptr);
        static_cast<T*>(ptr)->~T();

        byte_buf_.end_read();

        return true;
    }

private:
    SpscByteBuffer byte_buf_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SPSC_RING_BUFFER_H_
