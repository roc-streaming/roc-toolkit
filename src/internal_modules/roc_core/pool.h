/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/pool.h
//! @brief Pool.

#ifndef ROC_CORE_POOL_H_
#define ROC_CORE_POOL_H_

#include "roc_core/alignment.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/log.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Pool.
//!
//! @tparam T defines object type.
//!
//! Allocates chunks from given allocator containing a fixed number of fixed
//! sized objects. Maintains a list of free objects.
//!
//! The memory is always maximum aligned. Thread-safe.
template <class T> class Pool : public NonCopyable<> {
public:
    //! Initialization.
    //!
    //! @b Parameters
    //!  - @p allocator is used to allocate chunks
    //!  - @p object_size defines object size in bytes
    //!  - @p poison enables memory poisoning for debugging
    Pool(IAllocator& allocator, size_t object_size, bool poison)
        : allocator_(allocator)
        , used_elems_(0)
        , elem_size_(max_align(std::max(sizeof(Elem), object_size)))
        , chunk_hdr_size_(max_align(sizeof(Chunk)))
        , chunk_n_elems_(1)
        , poison_(poison) {
        roc_log(LogDebug, "pool: initializing: object_size=%lu poison=%d",
                (unsigned long)elem_size_, (int)poison);
    }

    ~Pool() {
        deallocate_all_();
    }

    //! Allocate new object.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory for a new object
    //!  or NULL if memory can't be allocated.
    void* allocate() {
        Elem* elem = get_elem_();
        if (elem == NULL) {
            return NULL;
        }

        elem->~Elem();

        void* memory = elem;

        if (poison_) {
            memset(memory, PoisonAllocated, elem_size_);
        } else {
            memset(memory, 0, elem_size_);
        }

        return memory;
    }

    //! Free previously allocated memory.
    void deallocate(void* memory) {
        if (memory == NULL) {
            roc_panic("pool: deallocating null pointer");
        }

        if (poison_) {
            memset(memory, PoisonDeallocated, elem_size_);
        }

        Elem* elem = new (memory) Elem;
        put_elem_(elem);
    }

    //! Destroy object and deallocate its memory.
    void destroy(T& object) {
        object.~T();
        deallocate(&object);
    }

private:
    enum { PoisonAllocated = 0x7a, PoisonDeallocated = 0x7d };

    struct Chunk : ListNode { };
    struct Elem : ListNode { };

    Elem* get_elem_() {
        Mutex::Lock lock(mutex_);

        if (free_elems_.size() == 0) {
            allocate_chunk_();
        }

        Elem* elem = free_elems_.front();
        if (elem != NULL) {
            free_elems_.remove(*elem);
            used_elems_++;
        }

        return elem;
    }

    void put_elem_(Elem* elem) {
        Mutex::Lock lock(mutex_);

        if (used_elems_ == 0) {
            roc_panic("pool: unpaired deallocation");
        }

        used_elems_--;
        free_elems_.push_front(*elem);
    }

    void allocate_chunk_() {
        void* memory = allocator_.allocate(chunk_offset_(chunk_n_elems_));
        if (memory == NULL) {
            return;
        }

        Chunk* chunk = new (memory) Chunk;
        chunks_.push_back(*chunk);

        for (size_t n = 0; n < chunk_n_elems_; n++) {
            Elem* elem = new ((char*)chunk + chunk_offset_(n)) Elem;
            free_elems_.push_back(*elem);
        }

        chunk_n_elems_ *= 2;
    }

    void deallocate_all_() {
        if (used_elems_ != 0) {
            roc_panic("pool: detected leak: used=%lu free=%lu",
                      (unsigned long)used_elems_, (unsigned long)free_elems_.size());
        }

        while (Elem* elem = free_elems_.front()) {
            free_elems_.remove(*elem);
        }

        while (Chunk* chunk = chunks_.front()) {
            chunks_.remove(*chunk);
            allocator_.deallocate(chunk);
        }
    }

    size_t chunk_offset_(size_t n) const {
        return chunk_hdr_size_ + n * elem_size_;
    }

    Mutex mutex_;

    IAllocator& allocator_;

    List<Chunk, NoOwnership> chunks_;
    List<Elem, NoOwnership> free_elems_;
    size_t used_elems_;

    const size_t elem_size_;
    const size_t chunk_hdr_size_;
    size_t chunk_n_elems_;

    const bool poison_;
};

} // namespace core
} // namespace roc

//! Placement new for core::Pool<T>.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
template <class T>
inline void* operator new(size_t size, roc::core::Pool<T>& pool) throw() {
    roc_panic_if(size != sizeof(T));
    return pool.allocate();
}

//! Placement delete for core::Pool<T>.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <class T>
inline void operator delete(void* ptr, roc::core::Pool<T>& pool) throw() {
    pool.deallocate(ptr);
}

#endif // ROC_CORE_POOL_H_
