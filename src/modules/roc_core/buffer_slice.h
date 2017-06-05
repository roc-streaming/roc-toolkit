/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer_slice.h
//! @brief Buffer slice.

#ifndef ROC_CORE_BUFFER_SLICE_H_
#define ROC_CORE_BUFFER_SLICE_H_

#include "roc_core/ibuffer.h"
#include "roc_core/panic.h"
#include "roc_core/print_buffer.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

//! Buffer slice.
template <class Element,
          class Buffer,
          template <class TT> class Ownership = RefCntOwnership>
class BufferSlice {
    typedef typename Ownership<Buffer>::SafePtr BufferPtr;

public:
    //! Construct empty slice.
    BufferSlice()
        : buffer_()
        , data_(NULL)
        , size_(0) {
    }

    //! Construct from buffer or const buffer.
    BufferSlice(Buffer& buffer)
        : buffer_(&buffer)
        , data_(buffer.data())
        , size_(buffer.size()) {
        check_();
    }

    //! Construct from buffer or const buffer.
    BufferSlice(Buffer& buffer, size_t off, size_t sz)
        : buffer_(&buffer)
        , data_(buffer.data() + off)
        , size_(sz) {
        check_();
    }

    //! Construct from slice or const slice.
    template <class E, class B> BufferSlice(const BufferSlice<E, B, Ownership>& slice) {
        // We need friend function since we can't make template slice our friend.
        copy_slice_(*this, slice, 0, slice.size());
    }

    //! Construct from slice or const slice.
    template <class E, class B>
    BufferSlice(const BufferSlice<E, B, Ownership>& slice, size_t off, size_t sz) {
        // We need friend function since we can't make template slice our friend.
        copy_slice_(*this, slice, off, sz);
    }

    ~BufferSlice() {
        if (buffer_) {
            check_();
        }
    }

    //! Get sliced data.
    Element* data() const {
        check_();

        return data_;
    }

    //! Get number of elements in slice.
    size_t size() const {
        return size_;
    }

    //! Print to stdout.
    void print() const {
        if (buffer_) {
            print_buffer(data(), size(), buffer_->max_size());
        } else {
            print_buffer((Element*)NULL, 0, 0);
        }
    }

    //! Convert to bool.
    operator const struct unspecified_bool*() const {
        return (const unspecified_bool*)buffer_.get();
    }

    //! Get container buffer.
    const BufferPtr& container() {
        return buffer_;
    }

private:
    template <class S1, class S2>
    friend void copy_slice_(S1& dst, const S2& src, size_t offset, size_t size);

    void check_() const {
        if (!buffer_) {
            roc_panic("slice buffer is null");
        }

        if (data_ == NULL) {
            roc_panic("slice data is null");
        }

        if (size_ == 0) {
            roc_panic("slice size is zero");
        }

        if (data_ < buffer_->data()
            || data_ + size_ > buffer_->data() + buffer_->size()) {
            roc_panic("slice out of buffer bounds "
                      "(offset = %ld, size = %ld, buffer_size = %ld)",
                      (long)(data_ - buffer_->data()), //
                      (long)size_,                     //
                      (long)buffer_->size());
        }

        buffer_->check();
    }

    BufferPtr buffer_;
    Element* data_;
    size_t size_;
};

//! BufferSlice constructor helper.
template <class S1, class S2>
void copy_slice_(S1& dst, const S2& src, size_t offset, size_t size) {
    if (offset + size > src.size_) {
        roc_panic("new slice out of original slice bounds "
                  "(offset = %ld, size = %ld, orig_size = %ld)",
                  (long)offset, //
                  (long)size,   //
                  (long)src.size_);
    }

    dst.buffer_ = src.buffer_;
    dst.data_ = src.data_ + offset;
    dst.size_ = size;

    dst.check_();
}

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_SLICE_H_
