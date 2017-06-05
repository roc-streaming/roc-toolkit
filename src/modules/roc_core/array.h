/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/array.h
//! @brief Array on stack.

#ifndef ROC_CORE_ARRAY_H_
#define ROC_CORE_ARRAY_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Array on stack.
template <class T, size_t MaxSz> class Array : public NonCopyable<> {
public:
    //! Initialize empty array.
    Array()
        : size_(0) {
    }

    //! Initialize array of given size.
    //! @see resize().
    explicit Array(size_t sz)
        : size_(0) {
        resize(sz);
    }

    ~Array() {
        resize(0);
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
            roc_panic("array subscript out of range (index = %lu, size = %lu)",
                      (unsigned long)index, (unsigned long)size_);
        }
        return storage_[index].ref();
    }

    //! Get element at given position.
    const T& operator[](size_t index) const {
        if (index >= size_) {
            roc_panic("array subscript out of range (index = %lu, size = %lu)",
                      (unsigned long)index, (unsigned long)size_);
        }
        return storage_[index].ref();
    }

    //! Get first element.
    //! @pre
    //!  Array should be non-empty.
    T& front() {
        if (size_ == 0) {
            roc_panic("attempting to call front() on empty array");
        }
        return storage_[0].ref();
    }

    //! Get first element.
    //! @pre
    //!  Array should be non-empty.
    const T& front() const {
        if (size_ == 0) {
            roc_panic("attempting to call front() on empty array");
        }
        return storage_[0].ref();
    }

    //! Get last element.
    //! @pre
    //!  Array should be non-empty.
    T& back() {
        if (size_ == 0) {
            roc_panic("attempting to call back() on empty array");
        }
        return storage_[size_ - 1].ref();
    }

    //! Get last element.
    //! @pre
    //!  Array should be non-empty.
    const T& back() const {
        if (size_ == 0) {
            roc_panic("attempting to call back() on empty array");
        }
        return storage_[size_ - 1].ref();
    }

    //! Set array size.
    //! @remarks
    //!  @p sz should be less than or equal to max_size().
    void resize(size_t sz) {
        if (sz > MaxSz) {
            roc_panic("attempting to call to resize() with too large size "
                      "(sz = %lu, max_size = %lu)",
                      (unsigned long)sz, (unsigned long)MaxSz);
        }

        /* Construct objects if size increased.
         */
        for (size_t n = size_; n < sz; n++) {
            new (storage_[n].mem()) T();
        }

        /* Destroy objects (in reverse oreder) if size decreased.
         */
        for (size_t n = size_; n > sz; n--) {
            storage_[n - 1].ref().~T();
        }

        size_ = sz;
    }

    //! Append element to array.
    //! @pre
    //!  Array size() should be less than max_size().
    void append(const T& value) {
        new (allocate()) T(value);
    }

    //! Allocate uninitialized memory for new element.
    //! @pre
    //!  Array size() should be less than max_size().
    //! @returns
    //!  Memory for new element that will be last in array.
    void* allocate() {
        if (size_ >= MaxSz) {
            roc_panic("attempting to append element to full array (size = %lu)",
                      (unsigned long)size_);
        }
        return storage_[size_++].mem();
    }

    //! Get raw memory.
    //! @remarks
    //!  May contain unitialized objects.
    T* memory() {
        return &storage_[0].ref();
    }

private:
    size_t size_;
    AlignedStorage<T> storage_[MaxSz];
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ARRAY_H_
