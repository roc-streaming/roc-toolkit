/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/slice.h
//! @brief Slice.

#ifndef ROC_CORE_SLICE_H_
#define ROC_CORE_SLICE_H_

#include "roc_core/buffer.h"
#include "roc_core/print_memory.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

//! Slice.
//!
//! Slice points to a subrange of data in pool-allocated Buffer.
//! Copying a slice produces a new slice referring the same data.
//!
//! Slice also acts as a kind of shared pointer to Buffer. A buffer won't be freed
//! (returned to pool) until there are slices referring it. Copying a slice
//! increments the buffer reference counter, and destroying a slice decrements it.
//!
//! While Buffer works with raw bytes, Slice<T> interprets it as array of elements
//! of type T, and works in terms of those elements.
//!
//! Slice has two important characteristics:
//!  - size - the difference between the ending end beginning pointer
//!  - capacity - the difference between the actual buffer end and the
//!               slice beginning pointer
//!
//! Buffers are not resizable. They're allocated from pool and have fixed size,
//! defined by the pool parameters.
//!
//! Slices are reslicable, which means that their pointers to the buffer data
//! may be moved within the buffer.
//!
//! The beginning pointer may be moved only forward. Once moved, it's not allowed
//! to move it backward again. Moving it decreases the slice size and capacity.
//! Capacity is affected because it's relative to the beginning pointer.
//!
//! The ending pointer may be freely moved forward and backward within the slice
//! capacity. Moving it affects the slice size, but not capacity.
//!
//! In other words, slice capacity may be only decreased by moving beginning pointer,
//! and slice size may be freely changed within the slice capacity by moving both
//! beginning and ending pointers.
template <class T> class Slice {
public:
    //! Construct empty slice.
    Slice()
        : buffer_()
        , data_(NULL)
        , size_(0) {
    }

    //! Construct slice pointing to the whole buffer.
    Slice(const BufferPtr& buffer) {
        buffer_ = buffer;
        if (buffer_) {
            data_ = (T*)buffer->data();
            size_ = buffer->size() / sizeof(T);
        } else {
            data_ = NULL;
            size_ = 0;
        }
    }

    //! Construct slice pointing to a part of a buffer.
    Slice(Buffer& buffer, size_t from, size_t to) {
        if (from > to) {
            roc_panic("slice: invalid range: [%lu,%lu)", (unsigned long)from,
                      (unsigned long)to);
        }
        if (to > buffer.size() / sizeof(T)) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=[%lu,%lu)",
                      (unsigned long)0, (unsigned long)buffer.size() / sizeof(T),
                      (unsigned long)from, (unsigned long)to);
        }
        buffer_ = &buffer;
        data_ = (T*)buffer.data() + from;
        size_ = to - from;
    }

    //! Get slice data.
    T* data() const {
        if (data_ == NULL) {
            roc_panic("slice: null slice");
        }
        return data_;
    }

    //! Pointer to the next after the last element in slice.
    T* data_end() const {
        if (data_ == NULL) {
            roc_panic("slice: null slice");
        }
        return data_ + size_;
    }

    //! Get number of elements in slice.
    size_t size() const {
        return size_;
    }

    //! Get maximum possible number of elements in slice.
    size_t capacity() const {
        if (data_ == NULL) {
            return 0;
        } else {
            return buffer_->size() / sizeof(T) - size_t(data_ - (T*)buffer_->data());
        }
    }

    //! Change slice beginning and ending inside the buffer.
    //! @remarks
    //!  - @p from and @p to are relative to slice beginning.
    //!  - @p to value can be up to capacity().
    void reslice(size_t from, size_t to) {
        const size_t cap = capacity();
        if (from > to) {
            roc_panic("slice: invalid range: [%lu,%lu)", (unsigned long)from,
                      (unsigned long)to);
        }
        if (to > cap) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=[%lu,%lu)",
                      (unsigned long)0, (unsigned long)cap, (unsigned long)from,
                      (unsigned long)to);
        }
        if (data_) {
            data_ = data_ + from;
            size_ = to - from;
        }
    }

    //! Increase size() by @p add_sz.
    //! @returns
    //!  Pointer to the first element of extended range.
    T* extend(const size_t add_sz) {
        if (data_ == NULL) {
            roc_panic("slice: null slice");
        }
        if (add_sz == 0) {
            roc_panic("slice: extend with zero size");
        }
        T* ret = data_ + size_;
        reslice(0, size() + add_sz);
        return ret;
    }

    //! Construct a slice pointing to a part of this slice.
    //! @remarks
    //!  - @p from and @p to are relative to slice beginning.
    //!  - @p to value can be up to size().
    Slice subslice(size_t from, size_t to) const {
        if (from > to) {
            roc_panic("slice: invalid range: [%lu,%lu)", (unsigned long)from,
                      (unsigned long)to);
        }
        if (to > size_) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=[%lu,%lu)",
                      (unsigned long)0, (unsigned long)size_, (unsigned long)from,
                      (unsigned long)to);
        }
        Slice ret;
        ret.buffer_ = buffer_;
        ret.data_ = data_ + from;
        ret.size_ = to - from;
        return ret;
    }

    //! Print slice to stderr.
    void print() const {
        if (buffer_) {
            core::print_memory_slice(data_, size_, (T*)buffer_->data(),
                                     buffer_->size() / sizeof(T));
        } else {
            core::print_memory_slice(data_, size_, NULL, 0);
        }
    }

    //! Access to an element of the Slice with an array style.
    T& operator[](const size_t i) const {
        if (data_ == NULL) {
            roc_panic("slice: null slice");
        }
        if (i > size_) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=%lu",
                      (unsigned long)0, (unsigned long)size_, (unsigned long)i);
        }
        return data_[i];
    }

    //! Convert to bool.
    //! @returns
    //!  true if the slice is attached to buffer, even if it has zero length.
    operator const struct unspecified_bool *() const {
        return (const unspecified_bool*)data_;
    }

private:
    BufferPtr buffer_;
    T* data_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SLICE_H_
