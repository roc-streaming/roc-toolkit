/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/array.h
//! @brief Dynamic array.

#ifndef ROC_CORE_ARRAY_H_
#define ROC_CORE_ARRAY_H_

#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Dynamic array.
template <class T> class Array : public NonCopyable<> {
public:
    //! Initialize empty array.
    //!
    //! @b Parameters
    //!  - @p allocator is used to allocate a fixed size chunk of memory for array
    //!  - @p max_sz defines maximum number of elements in array
    //!
    //! @remarks
    //!  The allocation happens exactly once when the array is initialize. The maximum
    //!  array size can't be changed after initialization.
    Array(IAllocator& allocator, size_t max_sz)
        : data_((T*)allocator.allocate(max_sz * sizeof(T)))
        , size_(0)
        , max_size_(max_sz)
        , allocator_(allocator) {
        if (data_ == NULL) {
            roc_panic("array: can't allocate memory, size=%lu",
                      (unsigned long)max_sz * sizeof(T));
        }
    }

    ~Array() {
        resize(0);
        allocator_.deallocate(data_);
    }

    //! Get maximum number of elements.
    size_t max_size() const {
        return max_size_;
    }

    //! Get number of elements.
    size_t size() const {
        return size_;
    }

    //! Get element at given position.
    T& operator[](size_t index) {
        if (index >= size_) {
            roc_panic("array: subscript out of range (index = %lu, size = %lu)",
                      (unsigned long)index, (unsigned long)size_);
        }
        return data_[index];
    }

    //! Get element at given position.
    const T& operator[](size_t index) const {
        if (index >= size_) {
            roc_panic("array: subscript out of range (index = %lu, size = %lu)",
                      (unsigned long)index, (unsigned long)size_);
        }
        return data_[index];
    }

    //! Get first element.
    //! @pre
    //!  Array should be non-empty.
    T& front() {
        if (size_ == 0) {
            roc_panic("array: attempting to call front() on empty array");
        }
        return data_[0];
    }

    //! Get first element.
    //! @pre
    //!  Array should be non-empty.
    const T& front() const {
        if (size_ == 0) {
            roc_panic("array: attempting to call front() on empty array");
        }
        return data_[0];
    }

    //! Get last element.
    //! @pre
    //!  Array should be non-empty.
    T& back() {
        if (size_ == 0) {
            roc_panic("array: attempting to call back() on empty array");
        }
        return data_[size_ - 1];
    }

    //! Get last element.
    //! @pre
    //!  Array should be non-empty.
    const T& back() const {
        if (size_ == 0) {
            roc_panic("array: attempting to call back() on empty array");
        }
        return data_[size_ - 1];
    }

    //! Set array size.
    //! @remarks
    //!  @p sz should be less than or equal to max_size().
    void resize(size_t sz) {
        if (sz > max_size_) {
            roc_panic("array: attempting to call to resize() with too large size: "
                      "sz=%lu, max_size=%lu",
                      (unsigned long)sz, (unsigned long)max_size_);
        }

        // Construct objects if size increased.
        for (size_t n = size_; n < sz; n++) {
            new (data_ + n) T();
        }

        // Destroy objects (in reverse oreder) if size decreased.
        for (size_t n = size_; n > sz; n--) {
            data_[n - 1].~T();
        }

        size_ = sz;
    }

    //! Append element to array.
    //! @pre
    //!  Array size() should be less than max_size().
    void push_back(const T& value) {
        new (allocate_back()) T(value);
    }

    //! Allocate uninitialized memory for new element at the end of array.
    //! @pre
    //!  Array size() should be less than max_size().
    //! @returns
    //!  Memory for new element that will be last in array.
    void* allocate_back() {
        if (size_ >= max_size_) {
            roc_panic("array: attempting to append element to full array (size = %lu)",
                      (unsigned long)size_);
        }
        return data_ + size_++;
    }

private:
    T* data_;
    size_t size_;
    size_t max_size_;
    IAllocator& allocator_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ARRAY_H_
