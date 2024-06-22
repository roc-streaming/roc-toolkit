/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ring_queue.h
//! @brief Queue on continuous memory buffer.

#ifndef ROC_CORE_RING_QUEUE_H_
#define ROC_CORE_RING_QUEUE_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/log.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Queue on continuous memory buffer.
//!
//! Elements are stored continuously in a memory chunk allocated using IArena,
//! or directly in Array object when number of elements is small.
//!
//! RingQueue supports inserting and removing elements to the beginning and to
//! the end with O(1) complexity.
//!
//! @tparam T defines array element type. It should have default constructor
//! and copy constructor.
//!
//! @tparam EmbeddedCapacity defines number of elements in the fixed-size chunk
//! embedded directly into RingQueue object.
template <class T, size_t EmbeddedCapacity = 0> class RingQueue : public NonCopyable<> {
public:
    //! Initialize.
    //! @remarks
    //!  Preallocate buffer in @p arena for a queue with a capacity of `max_len` elements.
    //!  In this implementation, an empty slot needs to be reserved in the buffer to be
    //!  able to distinguish between the queue's empty and full states, so `max_len + 1`
    //!  elements are allocated for the buffer.
    RingQueue(core::IArena& arena, size_t max_len)
        : buff_(NULL)
        , buff_len_(max_len + 1)
        , begin_(0)
        , end_(0)
        , arena_(arena) {
        if (max_len <= 0) {
            roc_panic("ring queue: the length must be greater than 0");
        }

        buff_ = allocate_(buff_len_);
    }

    ~RingQueue() {
        if (buff_) {
            deallocate_(buff_);
        }
    }

    //! Check that initial allocation succeeded.
    bool is_valid() const {
        return buff_ != NULL;
    }

    //! Get maximum number of elements in queue.
    size_t capacity() const {
        return buff_len_ - 1;
    }

    //! Get current number of elements in the queue.
    size_t size() const {
        return (end_ - begin_ + buff_len_) % buff_len_;
    }

    //! Is the queue empty.
    bool is_empty() {
        return begin_ == end_;
    }

    //! Is the queue full.
    bool is_full() {
        return begin_ == (end_ + 1) % buff_len_;
    }

    //! Get reference of the front element.
    //! @pre
    //!  Queue should not be empty.
    T& front() {
        if (is_empty()) {
            roc_panic("ring queue: front() called on empty buffer");
        }
        return buff_[begin_];
    }

    //! Get reference of the front element.
    //! @pre
    //!  Queue should not be empty.
    const T& front() const {
        if (is_empty()) {
            roc_panic("ring queue: front() called on empty buffer");
        }
        return buff_[begin_];
    }

    //! Get reference of the back element.
    //! @pre
    //!  Queue should not be empty.
    T& back() {
        if (is_empty()) {
            roc_panic("ring queue: back() called on empty buffer");
        }
        return buff_[(end_ - 1 + buff_len_) % buff_len_];
    }

    //! Get reference of the back element.
    //! @pre
    //!  Queue should not be empty.
    const T& back() const {
        if (is_empty()) {
            roc_panic("ring queue: back() called on empty buffer");
        }
        return buff_[(end_ - 1 + buff_len_) % buff_len_];
    }

    //! Push an element to the front of the queue.
    //! @pre
    //!  Queue should not be full.
    void push_front(const T& x) {
        if (is_full()) {
            roc_panic("ring queue: buffer overflow");
        }
        begin_ = (begin_ - 1 + buff_len_) % buff_len_;
        new (&buff_[begin_]) T(x);
    }

    //! Remove the first element from the front.
    //! @pre
    //!  Queue should not be empty.
    void pop_front() {
        if (is_empty()) {
            roc_panic("ring queue: pop_front() called on empty buffer");
        }
        buff_[begin_].~T();
        begin_ = (begin_ + 1) % buff_len_;
    }

    //! Push an element to the backside of the queue.
    //! @pre
    //!  Queue should not be full.
    void push_back(const T& x) {
        if (is_full()) {
            roc_panic("ring queue: buffer overflow");
        }
        new (&buff_[end_]) T(x);
        end_ = (end_ + 1) % buff_len_;
    }

    //! Remove the first element from the back.
    //! @pre
    //!  Queue should not be empty.
    void pop_back() {
        if (is_empty()) {
            roc_panic("ring queue: pop_back() called on empty buffer");
        }
        buff_[end_].~T();
        end_ = (end_ - 1 + buff_len_) % buff_len_;
    }

private:
    T* allocate_(size_t n_buff_elems) {
        T* data = NULL;

        // n_buff_elems - 1 = max_len = queue capacity; EmbeddedCapacity specifies the max
        // queue capacity that's allowed in order to have an embedded buffer. Embedding
        // will not be used when EmbeddedCapacity = 0 since max_len = buff_len_ - 1 will
        // always be strictly greater than 0; this is enforced in the constructor.
        if (n_buff_elems - 1 <= EmbeddedCapacity) {
            data = (T*)embedded_data_.memory();
        } else {
            data = (T*)arena_.allocate(n_buff_elems * sizeof(T));
        }

        if (!data) {
            roc_log(LogError,
                    "ring queue: can't allocate memory:"
                    " requested_cap=%lu embedded_cap=%lu",
                    (unsigned long)n_buff_elems, (unsigned long)EmbeddedCapacity);
        }

        return data;
    }

    void deallocate_(T* data) {
        if ((void*)data != (void*)embedded_data_.memory()) {
            arena_.deallocate(data);
        }
    }

    T* buff_;
    size_t buff_len_;
    size_t begin_;
    size_t end_;

    AlignedStorage<(EmbeddedCapacity != 0 ? EmbeddedCapacity + 1 : 0) * sizeof(T)>
        embedded_data_;
    IArena& arena_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_RING_QUEUE_H_
