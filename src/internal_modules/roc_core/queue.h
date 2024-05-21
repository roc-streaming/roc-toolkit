/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/queue.h
//! @brief Queue for trivially typed elements with continuous memory buffer.

#ifndef ROC_CORE_QUEUE_H_
#define ROC_CORE_QUEUE_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Queue on dynamic array.
//! @tparam T defines type of a single element.
//!
template <class T> class Queue {
public:
    //! Initialize.
    //! @remarks
    //! Preallocate buffer in @p arena with @p len number of elements.
    Queue(core::IArena& arena, size_t len)
        : buff_(arena)
        , buff_len_(len)
        , begin_(0)
        , end_(0) {
        if (len == 0) {
            roc_panic("queue: the length must be greater than 0");
        }

        if (!buff_.resize(len)) {
            roc_panic("queue: can't allocate storage for the buffer");
        }
    }

    //! Get reference of the front element.
    T& front() {
        if (is_empty()) {
            roc_panic("queue: front() called on empty buffer");
        }
        return buff_[begin_];
    }

    //! Get reference of the back element.
    T& back() {
        if (is_empty()) {
            roc_panic("queue: back() called on empty buffer");
        }
        return buff_[(end_ - 1 + buff_len_) % buff_len_];
    }

    //! Get number of elements in the queue.
    size_t len() const {
        return (end_ - begin_ + buff_len_) % buff_len_;
    }

    //! Push an element to the front of the queue.
    void push_front(const T& x) {
        begin_ = (begin_ - 1 + buff_len_) % buff_len_;
        buff_[begin_] = x;
        roc_panic_if_msg(end_ == begin_, "queue: buffer overflow");
    }

    //! Remove the first element from the front.
    void pop_front() {
        if (is_empty()) {
            roc_panic("queue: pop_front() called on empty buffer");
        }
        begin_ = (begin_ + 1) % buff_len_;
    }

    //! Push an element to the backside of the queue.
    void push_back(const T& x) {
        buff_[end_] = x;
        end_ = (end_ + 1) % buff_len_;
        roc_panic_if_msg(end_ == begin_, "queue: buffer overflow");
    }

    //! Remove the first element from the back.
    void pop_back() {
        if (is_empty()) {
            roc_panic("queue: pop_back() called on empty buffer");
        }
        end_ = (end_ - 1 + buff_len_) % buff_len_;
    }

    //! Is the queue empty.
    bool is_empty() {
        return begin_ == end_;
    }

private:
    Array<T> buff_;
    size_t buff_len_;
    size_t begin_;
    size_t end_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_QUEUE_H_
