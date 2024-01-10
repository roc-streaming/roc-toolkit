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
//! Elements are stored continuously in a memory chunk allocated using IArena.
//! Small chunks can be stored directly in Array object, without extra allocation.
//! Array can be resized only by explicitly calling resize(), grow(), or grow_exp().
//! Elements are copied during resize and old copies are destroyed.
//!
//! @tparam T defines array element type. It should have copy constructor and
//! destructor.
//!
//! @tparam EmbeddedCapacity defines the size of the fixed-size array embedded
//! directly into Array object; it is used instead of dynamic memory if
//! the array size is small enough.
template <class T, size_t EmbeddedCapacity = 0> class Array : public NonCopyable<> {
public:
    //! Initialize empty array without arena.
    //! @remarks
    //!  Array capacity will be limited to the embedded capacity.
    Array()
        : data_(NULL)
        , size_(0)
        , max_size_(0)
        , arena_(NULL) {
    }

    //! Initialize empty array with arena.
    //! @remarks
    //!  Array capacity may grow using arena.
    explicit Array(IArena& arena)
        : data_(NULL)
        , size_(0)
        , max_size_(0)
        , arena_(&arena) {
    }

    ~Array() {
        clear();

        if (data_) {
            deallocate_(data_);
        }
    }

    //! Get maximum number of elements.
    //! If array has arena, capacity can be grown.
    size_t capacity() const {
        return max_size_;
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

    //! Append element to array.
    //! @returns
    //!  false if the allocation failed
    ROC_ATTR_NODISCARD bool push_back(const T& value) {
        if (!grow_exp(size() + 1)) {
            return false;
        }

        new (data_ + size_) T(value);
        size_++;

        return true;
    }

    //! Set array size.
    //! @remarks
    //!  Calls grow() to ensure that there is enough space in array.
    //! @returns
    //!  false if the allocation failed
    ROC_ATTR_NODISCARD bool resize(size_t sz) {
        // Move objects to a new memory region if necessary.
        if (!grow(sz)) {
            return false;
        }

        // Construct new objects if size increased.
        for (size_t n = size_; n < sz; n++) {
            new (data_ + n) T();
        }

        // Destruct old objects (in reverse order) if size decreased.
        for (size_t n = size_; n > sz; n--) {
            data_[n - 1].~T();
        }

        size_ = sz;

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
    //!  If @p max_sz is greater than the current capacity, a larger memory
    //!  region is allocated and the array elements are copied there.
    //! @returns
    //!  false if the allocation failed
    ROC_ATTR_NODISCARD bool grow(size_t max_sz) {
        if (max_sz <= max_size_) {
            return true;
        }

        T* new_data = allocate_(max_sz);
        if (!new_data) {
            roc_log(LogError, "array: can't allocate memory: old_size=%lu new_size=%lu",
                    (unsigned long)max_size_, (unsigned long)max_sz);
            return false;
        }

        if (new_data != data_) {
            // Copy old objects to new memory.
            for (size_t n = 0; n < size_; n++) {
                new (new_data + n) T(data_[n]);
            }

            // Destruct objects in old memory (in reverse order).
            for (size_t n = size_; n > 0; n--) {
                data_[n - 1].~T();
            }

            // Free old memory.
            if (data_) {
                deallocate_(data_);
            }

            data_ = new_data;
        }

        max_size_ = max_sz;
        return true;
    }

    //! Increase array capacity exponentially.
    //! @remarks
    //!  If @p min_size is greater than the current capacity, a larger memory
    //!  region is allocated and the array elements are copied there.
    //!  The size growth will follow the sequence: 0, 2, 4, 8, 16, ... until
    //!  it reaches some threshold, and then starts growing linearly.
    //! @returns
    //!  false if the allocation failed
    ROC_ATTR_NODISCARD bool grow_exp(size_t min_size) {
        if (min_size <= max_size_) {
            return true;
        }

        size_t new_max_size_ = max_size_;

        if (max_size_ < 1024) {
            while (min_size > new_max_size_) {
                new_max_size_ = (new_max_size_ == 0) ? 2 : new_max_size_ * 2;
            }
        } else {
            while (min_size > new_max_size_) {
                new_max_size_ += new_max_size_ / 4;
            }
        }

        return grow(new_max_size_);
    }

    //! Remove last element from the array.
    void pop_back() {
        if (size_ == 0) {
            roc_panic("array: array is empty");
        }

        // Destruct object
        data_[size_ - 1].~T();
        size_--;
    }

private:
    T* allocate_(size_t n_elems) {
        if (n_elems <= EmbeddedCapacity) {
            return (T*)embedded_data_.memory();
        } else if (arena_) {
            return (T*)arena_->allocate(n_elems * sizeof(T));
        } else {
            return NULL;
        }
    }

    void deallocate_(T* data) {
        if ((void*)data != (void*)embedded_data_.memory()) {
            roc_panic_if(!arena_);
            arena_->deallocate(data);
        }
    }

    T* data_;
    size_t size_;
    size_t max_size_;

    IArena* arena_;

    AlignedStorage<EmbeddedCapacity * sizeof(T)> embedded_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ARRAY_H_
