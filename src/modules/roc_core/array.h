/*
 * Copyright (c) 2015 Roc authors
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
#include "roc_core/log.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Dynamic array.
template <class T> class Array : public NonCopyable<> {
public:
    //! Initialize empty array.
    explicit Array(IAllocator& allocator)
        : data_(NULL)
        , size_(0)
        , max_size_(0)
        , allocator_(allocator) {
    }

    ~Array() {
        resize(0);

        if (data_) {
            allocator_.deallocate(data_);
        }
    }

    //! Get maximum number of elements.
    size_t max_size() const {
        return max_size_;
    }

    //! Get number of elements.
    size_t size() const {
        return size_;
    }

    //! Get pointer to first element.
    //! @remarks
    //!  Returns null if the array is empty.
    T* data() {
        if (size_) {
            return data_;
        } else {
            return NULL;
        }
    }

    //! Get pointer to first element.
    //! @remarks
    //!  Returns null if the array is empty.
    const T* data() const {
        if (size_) {
            return data_;
        } else {
            return NULL;
        }
    }

    //! Get element at given position.
    T& operator[](size_t index) {
        if (index >= size_) {
            roc_panic("array: subscript out of range: index=%lu size=%lu",
                      (unsigned long)index, (unsigned long)size_);
        }
        return data_[index];
    }

    //! Get element at given position.
    const T& operator[](size_t index) const {
        if (index >= size_) {
            roc_panic("array: subscript out of range: index=%lu size=%lu",
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

    //! Append element to array.
    //! @pre
    //!  Array size() should be less than max_size().
    void push_back(const T& value) {
        if (size_ >= max_size_) {
            roc_panic("array: attempting to append element to full array: size=%lu",
                      (unsigned long)size_);
        }
        new (data_ + size_) T(value);
        size_++;
    }

    //! Set array size.
    //! @remarks
    //!  Calls grow() to ensure that there is enough space in array.
    //! @returns
    //!  false if the allocation failed
    bool resize(size_t sz) {
        // Move objects to a new memory region if necessary.
        if (!grow(sz)) {
            return false;
        }

        // Construct objects if size increased.
        for (size_t n = size_; n < sz; n++) {
            new (data_ + n) T();
        }

        // Destruct objects (in reverse order) if size decreased.
        for (size_t n = size_; n > sz; n--) {
            data_[n - 1].~T();
        }

        size_ = sz;

        return true;
    }

    //! Increase array maximum size.
    //! @remarks
    //!  If @p max_sz is greater than the current maximum size, a larger memory
    //!  region is allocated and the array elements are copied there.
    //! @returns
    //!  false if the allocation failed
    bool grow(size_t max_sz) {
        if (max_sz <= max_size_) {
            return true;
        }

        T* new_data = (T*)allocator_.allocate(max_sz * sizeof(T));
        if (!new_data) {
            roc_log(LogError, "array: can't allocate memory: old_size=%lu new_size=%lu",
                    (unsigned long)max_size_, (unsigned long)max_sz);
            return false;
        }

        // Copy objects.
        for (size_t n = 0; n < size_; n++) {
            new (new_data + n) T(data_[n]);
        }

        // Destruct objects (in reverse order).
        for (size_t n = size_; n > 0; n--) {
            data_[n - 1].~T();
        }

        if (data_) {
            allocator_.deallocate(data_);
        }

        data_ = new_data;
        max_size_ = max_sz;

        return true;
    }

    //! Increase exponentially array maximum size.
    //! @remarks
    //!  If @p min_size is greater than the current maximum size, a larger memory
    //!  region is allocated and the array elements are copied there.
    //!  The size growth will follow the sequence: 0, 2, 4, 8, 16, ...
    //! @returns
    //!  false if the allocation failed
    bool grow_exp(size_t min_size) {
        if (min_size <= max_size_) {
            return true;
        }

        size_t new_size = size_;
        while (min_size > new_size) {
            new_size = (new_size == 0) ? 2 : new_size * 2;
        }

        return grow(new_size);
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
