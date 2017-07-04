/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
#include "roc_core/macros.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

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
    //!  - @p obj_sz defines object size in bytes
    //!  - @p n_objs defines number of objects in a chunk
    Pool(IAllocator& allocator, size_t obj_sz, size_t n_objs)
        : allocator_(allocator)
        , obj_off_(max_align(sizeof(Chunk)))
        , obj_sz_(max_align(ROC_MAX(sizeof(Elem), obj_sz)))
        , n_objs_(n_objs) {
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
        return elem;
    }

    //! Free previously allocated memory.
    void deallocate(void* memory) {
        if (memory == NULL) {
            roc_panic("pool: null pointer");
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
    struct Chunk : ListNode {};
    struct Elem : ListNode {};

    Elem* get_elem_() {
        Mutex::Lock lock(mutex_);

        if (free_elems_.size() == 0) {
            allocate_chunk_();
        }

        Elem* elem = free_elems_.back();
        if (elem != NULL) {
            free_elems_.remove(*elem);
        }

        return elem;
    }

    void put_elem_(Elem* elem) {
        Mutex::Lock lock(mutex_);
        free_elems_.push_back(*elem);
    }

    void allocate_chunk_() {
        void* memory = allocator_.allocate(obj_off_ + obj_sz_ * n_objs_);
        if (memory == NULL) {
            return;
        }

        Chunk* chunk = new (memory) Chunk;
        chunks_.push_back(*chunk);

        for (size_t n = 0; n < n_objs_; n++) {
            Elem* elem = new ((char*)chunk + obj_off_ + obj_sz_ * n) Elem;
            free_elems_.push_back(*elem);
        }
    }

    void deallocate_all_() {
        if (free_elems_.size() != chunks_.size() * n_objs_) {
            roc_panic("pool: detected leak, avail=%lu, total=%lu",
                      (unsigned long)free_elems_.size(),
                      (unsigned long)(chunks_.size() * n_objs_));
        }

        while (Elem* elem = free_elems_.front()) {
            free_elems_.remove(*elem);
        }

        while (Chunk* chunk = chunks_.front()) {
            chunks_.remove(*chunk);
            allocator_.deallocate(chunk);
        }
    }

    Mutex mutex_;

    List<Chunk, NoOwnership> chunks_;
    List<Elem, NoOwnership> free_elems_;

    IAllocator& allocator_;
    size_t obj_off_;
    size_t obj_sz_;
    size_t n_objs_;
};

} // namespace core
} // namespace roc

//! Placement new for core::Pool<T>.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
template <class T>
inline void* operator new(size_t size, roc::core::Pool<T>& pool) ROC_ATTR_NOTHROW {
    roc_panic_if(size != sizeof(T));
    return pool.allocate();
}

//! Placement delete for core::Pool<T>.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <class T>
inline void operator delete(void* ptr, roc::core::Pool<T>& pool)ROC_ATTR_NOTHROW {
    pool.deallocate(ptr);
}

#endif // ROC_CORE_POOL_H_
