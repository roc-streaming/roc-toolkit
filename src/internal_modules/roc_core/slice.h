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
#include "roc_core/buffer_view.h"
#include "roc_core/print_memory.h"

namespace roc {
namespace core {

//! Slice.
//!
//! Slice points to a subrange of data either in Buffer (allocated from pool,
//! owns data) or BufferView (doesn't manage allocation, doesn't own data).
//! Copying a slice produces a new slice referring the same data.
//!
//! Slice also acts as a kind of shared pointer to Buffer or BufferView. Copying a
//! slice increments the reference counter, and destroying a slice decrements it.
//! Buffer uses reference counter to release itself to pool, and buffer view uses it
//! to check that its memory is no more referenced upon buffer view destruction.
//!
//! While Buffer and BufferView work with raw bytes, Slice<T> interprets memory as array
//! of elements of @tparam T, and its operations work in terms of those elements.
//!
//! Slice has a few important properties:
//!  - beginning and ending pointers - data inside underlying buffer or buffer view
//!  - size - number of elements between ending and beginning pointers
//!  - capacity - number of elements between the actual buffer or buffer view ending,
//!               and the slice beginning pointer
//!
//! Buffers and BufferViews are not resizable. Their size is defined at construction time.
//! Slices, on the other hand, are *reslicable*, which means that their pointers to
//! the data may be moved within the available capacity.
//!
//! The beginning pointer may be moved only forward. Once moved, it's not allowed
//! to move it backward again. Moving it decreases both slice size and capacity.
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
    Slice() {
        data_ = NULL;
        size_ = 0;
    }

    //! Construct slice pointing to the whole buffer.
    Slice(const BufferPtr& buf) {
        if (buf) {
            buffer_ = buf;
            data_ = buf_data_(*buf);
            size_ = buf_size_(*buf);
        } else {
            data_ = NULL;
            size_ = 0;
        }
    }

    //! Construct slice pointing to a part of a buffer.
    Slice(Buffer& buf, size_t from, size_t to) {
        const size_t max = buf_size_(buf);

        if (from > to) {
            roc_panic("slice: invalid range: [%lu,%lu)", (unsigned long)from,
                      (unsigned long)to);
        }
        if (to > max) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=[%lu,%lu)",
                      (unsigned long)0, (unsigned long)max, (unsigned long)from,
                      (unsigned long)to);
        }

        buffer_ = &buf;
        data_ = buf_data_(buf) + from;
        size_ = to - from;
    }

    //! Construct slice pointing to the whole buffer view.
    Slice(BufferView& buf_view) {
        view_ = &buf_view;
        data_ = view_data_(buf_view);
        size_ = view_size_(buf_view);
    }

    //! Construct slice pointing to a part of a buffer view.
    Slice(BufferView& buf_view, size_t from, size_t to) {
        const size_t max = view_size_(buf_view);

        if (from > to) {
            roc_panic("slice: invalid range: [%lu,%lu)", (unsigned long)from,
                      (unsigned long)to);
        }
        if (to > max) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=[%lu,%lu)",
                      (unsigned long)0, (unsigned long)max, (unsigned long)from,
                      (unsigned long)to);
        }

        view_ = &buf_view;
        data_ = view_data_(buf_view) + from;
        size_ = to - from;
    }

    //! Reset slice to empty state.
    void reset() {
        buffer_.reset();
        view_.reset();
        data_ = NULL;
        size_ = 0;
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
        return container_size_() - size_t(data_ - container_data_());
    }

    //! Change slice beginning and ending inside the buffer.
    //! @remarks
    //!  - @p from and @p to are relative to slice beginning.
    //!  - @p to value can be up to capacity().
    void reslice(size_t from, size_t to) {
        const size_t max = capacity();

        if (from > to) {
            roc_panic("slice: invalid range: [%lu,%lu)", (unsigned long)from,
                      (unsigned long)to);
        }
        if (to > max) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=[%lu,%lu)",
                      (unsigned long)0, (unsigned long)max, (unsigned long)from,
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
        reslice(0, size_ + add_sz);
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
        ret.view_ = view_;
        ret.data_ = data_ + from;
        ret.size_ = to - from;
        return ret;
    }

    //! Print slice to stderr.
    void print() const {
        core::print_memory_slice(data_, size_, container_data_(), container_size_());
    }

    //! Access to an element of the Slice with an array style.
    T& operator[](const size_t index) const {
        if (index > size_) {
            roc_panic("slice: out of bounds: available=[%lu,%lu) requested=%lu",
                      (unsigned long)0, (unsigned long)size_, (unsigned long)index);
        }

        return data_[index];
    }

    //! Convert to bool.
    //! @returns
    //!  true if the slice is attached to buffer, even if it has zero length.
    operator const struct unspecified_bool *() const {
        return (const unspecified_bool*)data_;
    }

private:
    static size_t buf_size_(Buffer& buf) {
        return buf.size() / sizeof(T);
    }

    static T* buf_data_(Buffer& buf) {
        return (T*)buf.data();
    }

    static size_t view_size_(BufferView& buf_view) {
        return buf_view.size() / sizeof(T);
    }

    static T* view_data_(BufferView& buf_view) {
        return (T*)buf_view.data();
    }

    size_t container_size_() const {
        return buffer_ ? buf_size_(*buffer_) : view_ ? view_size_(*view_) : 0;
    }

    T* container_data_() const {
        return buffer_ ? buf_data_(*buffer_) : view_ ? view_data_(*view_) : NULL;
    }

    // only one can be set, either buffer or buffer view
    BufferPtr buffer_;
    BufferViewPtr view_;

    T* data_;     // start of region inside buffer or buffer view
    size_t size_; // size of region
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SLICE_H_
