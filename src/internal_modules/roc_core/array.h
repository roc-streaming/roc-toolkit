/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/array.h
//! @brief Dynamic array.

#ifndef ROC_CORE_ARRAY_H_
#define ROC_CORE_ARRAY_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/log.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Dynamic array.
//!
//! Elements are stored continuously in a memory chunk allocated using IArena,
//! or directly in Array object when number of elements is small.
//!
//! Array supports resizing and inserting and removing elements in the end with
//! amortized O(1) complexity.
//!
//! @tparam T defines array element type. It should have default constructor,
//! copy constructor, and assignment operator.
//!
//! @tparam EmbeddedCapacity defines number of elements in the fixed-size chunk
//! embedded directly into Array object; it is used instead of dynamic memory if
//! the array size is small enough.
template <class T, size_t EmbeddedCapacity = 0> class Array : public NonCopyable<> {
public:
    //! Initialize empty array with arena.
    //! @remarks
    //!  Array capacity may grow using arena.
    explicit Array(IArena& arena)
        : data_(NULL)
        , size_(0)
        , capacity_(0)
        , arena_(arena) {
    }

    ~Array() {
        clear();

        if (data_) {
            deallocate_(data_);
        }
    }

    //! Get maximum number of elements that can be added without reallocation.
    size_t capacity() const {
        return capacity_;
    }

    //! Get number of elements.
    size_t size() const {
        return size_;
    }

    //! Check if size is zero.
    bool is_empty() const {
        return size_ == 0;
    }

    //! Get pointer to first element.
    //! @remarks
    //!  Returns null if the array is empty.
    T* data() {
        if (size_ == 0) {
            roc_panic("array: is empty");
        }
        return data_;
    }

    //! Get pointer to first element.
    //! @remarks
    //!  Returns null if the array is empty.
    const T* data() const {
        if (size_ == 0) {
            roc_panic("array: is empty");
        }
        return data_;
    }

    //! Get element at given position.
    //! @pre
    //!  Panics if index is out of bounds.
    T& operator[](size_t index) {
        roc_panic_if_msg(index >= size_,
                         "array: subscript out of range: index=%lu size=%lu",
                         (unsigned long)index, (unsigned long)size_);

        return data_[index];
    }

    //! Get element at given position.
    //! @pre
    //!  Panics if index is out of bounds.
    const T& operator[](size_t index) const {
        roc_panic_if_msg(index >= size_,
                         "array: subscript out of range: index=%lu size=%lu",
                         (unsigned long)index, (unsigned long)size_);

        return data_[index];
    }

    //! Get reference to first element.
    T& front() {
        if (size_ == 0) {
            roc_panic("array: is empty");
        }
        return data_[0];
    }

    //! Get const reference to first element.
    const T& front() const {
        if (size_ == 0) {
            roc_panic("array: is empty");
        }
        return data_[0];
    }

    //! Get reference to last element.
    T& back() {
        if (size_ == 0) {
            roc_panic("array: is empty");
        }
        return data_[size_ - 1];
    }

    //! Get const reference to last element.
    const T& back() const {
        if (size_ == 0) {
            roc_panic("array: is empty");
        }
        return data_[size_ - 1];
    }

    //! Append element to array.
    //! @returns
    //!  false if the allocation failed.
    //! @note
    //!  has amortized O(1) complexity, O(n) in worst case.
    ROC_ATTR_NODISCARD bool push_back(const T& value) {
        if (!grow_exp(size_ + 1)) {
            return false;
        }

        new (data_ + size_) T(value);
        size_++;

        return true;
    }

    //! Remove last element from the array.
    //! @pre
    //!  Panics if array is empty.
    void pop_back() {
        if (size_ == 0) {
            roc_panic("array: array is empty");
        }

        // Destruct object
        data_[size_ - 1].~T();
        size_--;
    }

    //! Set array size.
    //! @remarks
    //!  Calls grow() to ensure that there is enough space in array.
    //! @returns
    //!  false if the allocation failed
    ROC_ATTR_NODISCARD bool resize(size_t new_size) {
        // Move objects to a new memory region if necessary.
        if (!grow(new_size)) {
            return false;
        }

        // Construct new objects if size increased.
        for (size_t n = size_; n < new_size; n++) {
            new (data_ + n) T();
        }

        // Destruct old objects (in reversed order) if size decreased.
        for (size_t n = size_; n > new_size; n--) {
            data_[n - 1].~T();
        }

        size_ = new_size;

        return true;
    }

    //! Set array size to zero.
    //! @remarks
    //!  Never fails.
    void clear() {
        (void)resize(0);
    }

    //! Increase array capacity.
    //! @remarks
    //!  If @p min_capacity is greater than the current capacity, a larger memory
    //!  region is allocated and the array elements are copied there.
    //! @returns
    //!  false if the allocation failed.
    ROC_ATTR_NODISCARD bool grow(size_t min_capacity) {
        if (min_capacity <= capacity_) {
            return true;
        }

        T* new_data = allocate_(min_capacity);
        if (!new_data) {
            return false;
        }

        if (new_data != data_) {
            // Copy old objects to new memory.
            for (size_t n = 0; n < size_; n++) {
                new (new_data + n) T(data_[n]);
            }

            // Destruct objects in old memory (in reversed order).
            for (size_t n = size_; n > 0; n--) {
                data_[n - 1].~T();
            }

            // Free old memory.
            if (data_) {
                deallocate_(data_);
            }

            data_ = new_data;
        }

        capacity_ = min_capacity;
        return true;
    }

    //! Increase array capacity exponentially.
    //! @remarks
    //!  If @p min_capacity is greater than the current capacity, a larger memory
    //!  region is allocated and the array elements are copied there.
    //!  The size growth will follow the sequence: 0, 2, 4, 8, 16, ... until
    //!  it reaches some threshold, and then starts growing linearly.
    //! @returns
    //!  false if the allocation failed.
    ROC_ATTR_NODISCARD bool grow_exp(size_t min_capacity) {
        if (min_capacity <= capacity_) {
            return true;
        }

        const size_t new_capacity = next_capacity_(min_capacity);

        return grow(new_capacity);
    }

private:
    T* allocate_(size_t n_elems) {
        T* data = NULL;

        if (n_elems <= EmbeddedCapacity) {
            data = (T*)embedded_data_.memory();
        } else {
            data = (T*)arena_.allocate(n_elems * sizeof(T));
        }

        if (!data) {
            roc_log(LogError,
                    "array: can't allocate memory:"
                    " current_cap=%lu requested_cap=%lu embedded_cap=%lu",
                    (unsigned long)capacity_, (unsigned long)n_elems,
                    (unsigned long)EmbeddedCapacity);
        }

        return data;
    }

    void deallocate_(T* data) {
        if ((void*)data != (void*)embedded_data_.memory()) {
            arena_.deallocate(data);
        }
    }

    size_t next_capacity_(size_t min_size) const {
        size_t new_capacity = capacity_;

        if (capacity_ < 1024) {
            while (min_size > new_capacity) {
                new_capacity = (new_capacity == 0) ? 2 : new_capacity * 2;
            }
        } else {
            while (min_size > new_capacity) {
                new_capacity += new_capacity / 4;
            }
        }

        return new_capacity;
    }

    T* data_;
    size_t size_;
    size_t capacity_;

    IArena& arena_;

    AlignedStorage<EmbeddedCapacity * sizeof(T)> embedded_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ARRAY_H_
