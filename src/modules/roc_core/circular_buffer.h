/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/circular_buffer.h
//! @brief Circular buffer.

#ifndef ROC_CORE_CIRCULAR_BUFFER_H_
#define ROC_CORE_CIRCULAR_BUFFER_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Circular buffer.
template <class T, size_t MaxSz> class CircularBuffer : public NonCopyable<> {
public:
    CircularBuffer()
        : offset_(0)
        , size_(0) {
    }

    //! Construct circular buffer of given size.
    explicit CircularBuffer(size_t sz)
        : offset_(0)
        , size_(sz) {
        if (size_ > MaxSz) {
            roc_panic("attempting to create circular buffer larger than maximum size "
                      "(size = %lu, max_size = %lu)",
                      (unsigned long)size_, (unsigned long)MaxSz);
        }

        for (size_t n = 0; n < size_; n++) {
            new (storage_[n].mem()) T();
        }
    }

    ~CircularBuffer() {
        clear();
    }

    //! Get number of elements.
    size_t size() const {
        return size_;
    }

    //! Get maximum number of elements.
    size_t max_size() const {
        return MaxSz;
    }

    //! Get element at given position.
    T& operator[](size_t index) {
        if (index >= size_) {
            roc_panic("circular buffer subscript out of range (index = %lu, size = %lu)",
                      (unsigned long)index, (unsigned long)size_);
        }
        return storage_[(offset_ + index) % MaxSz].ref();
    }

    //! Get element at given position.
    const T& operator[](size_t index) const {
        if (index >= size_) {
            roc_panic("circular buffer subscript out of range (index = %lu, size = %lu)",
                      (unsigned long)index, (unsigned long)size_);
        }
        return storage_[(offset_ + index) % MaxSz].ref();
    }

    //! Get first element.
    //! @pre
    //!  Circular buffer should be non-empty.
    T& front() {
        if (size_ == 0) {
            roc_panic("attempting to call front() on empty circular buffer");
        }
        return storage_[offset_].ref();
    }

    //! Get first element.
    //! @pre
    //!  Circular buffer should be non-empty.
    const T& front() const {
        if (size_ == 0) {
            roc_panic("attempting to call front() on empty circular buffer");
        }
        return storage_[offset_].ref();
    }

    //! Get last element.
    //! @pre
    //!  Circular buffer should be non-empty.
    T& back() {
        if (size_ == 0) {
            roc_panic("attempting to call back() on empty circular buffer");
        }
        return storage_[(offset_ + size_ - 1) % MaxSz].ref();
    }

    //! Get last element.
    //! @pre
    //!  Circular buffer should be non-empty.
    const T& back() const {
        if (size_ == 0) {
            roc_panic("attempting to call back() on empty circular buffer");
        }
        return storage_[(offset_ + size_ - 1) % MaxSz].ref();
    }

    //! Append element.
    //! @remarks
    //!  If circular buffer is full, overwrites first element.
    void push(const T& value) {
        size_t index = (offset_ + size_) % MaxSz;

        if (size_ < MaxSz) {
            new (storage_[index].mem()) T(value);
            size_++;
        } else {
            storage_[index].ref() = value;
            offset_ = ((offset_ + 1) % MaxSz);
        }
    }

    //! Remove and return first element.
    //! @pre
    //!  Circular buffer should be non-empty.
    T shift() {
        if (size_ == 0) {
            roc_panic("attempting to call pop() on empty circular buffer");
        }

        T ret = storage_[offset_].ref();
        storage_[offset_].ref().~T();

        offset_ = ((offset_ + 1) % MaxSz);
        size_--;
        return ret;
    }

    //! Rotate circular buffer.
    //! @pre
    //!  Circular buffer should be full (size() == max_size()).
    //! @remarks
    //!  For example, after rotate(2) this circular buffer:
    //!  @code
    //!   1 2 3 4 5
    //!  @endcode
    //!  becomes:
    //!  @code
    //!   3 4 5 1 2
    //!  @endcode
    void rotate(size_t n) {
        if (size_ != MaxSz) {
            roc_panic("attempting to call rotate() on non-full circular buffer");
        }
        offset_ = ((offset_ + n) % MaxSz);
    }

    //! Clear circular buffer.
    void clear() {
        for (size_t n = 0; n < size_; n++) {
            (*this)[n].~T();
        }
        size_ = 0;
    }

    //! Get raw memory.
    //! @remarks
    //!  May contain unitialized objects.
    T* memory() {
        return &storage_[0].ref();
    }

private:
    size_t offset_;
    size_t size_;
    AlignedStorage<T> storage_[MaxSz];
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_CIRCULAR_BUFFER_H_
