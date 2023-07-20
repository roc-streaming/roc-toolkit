/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/slab_pool.h
//! @brief Slab pool.

#ifndef ROC_CORE_SLAB_POOL_H_
#define ROC_CORE_SLAB_POOL_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/attributes.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slab_pool_impl.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Slab pool.
//!
//! Allocates large chunks of memory ("slabs") from given allocator, and use them for
//! multiple smaller fixed-sized objects ("slots").
//!
//! Keeps track of free slots and use them when possible. Automatically allocates new
//! slabs when there are no free slots available.
//!
//! Automatically grows size of new slabs exponentially. The user can also specify the
//! minimum and maximum limits for the slabs.
//!
//! The returned memory is always maximum-aligned. Thread-safe.
//!
//! Supports memory "poisoning" to make memory-related bugs (out of bound writes, use
//! after free, etc) more noticeable.
//!
//! @tparam EmbeddedCapacity defines number of bytes embedded directly into SlabPool
//! object. If non-zero, these bytes will be used for first allocations, before using
//! actual allocator.
template <size_t EmbeddedCapacity = 0> class SlabPool : public NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p allocator is used to allocate slabs
    //!  - @p object_size defines size of single object in bytes
    //!  - @p min_alloc_bytes defines minimum size in bytes per request to allocator
    //!  - @p max_alloc_bytes defines maximum size in bytes per request to allocator
    //!  - @p poison enables memory poisoning for debugging
    SlabPool(IAllocator& allocator,
             size_t object_size,
             bool poison,
             size_t min_alloc_bytes = 0,
             size_t max_alloc_bytes = 0)
        : impl_(allocator,
                object_size,
                poison,
                min_alloc_bytes,
                max_alloc_bytes,
                embedded_data_.memory(),
                embedded_data_.size()) {
    }

    //! Get size of objects in pool.
    size_t object_size() const {
        return impl_.object_size();
    }

    //! Reserve memory for given number of objects.
    //! @returns
    //!  false if allocation failed.
    ROC_ATTR_NODISCARD bool reserve(size_t n_objects) {
        return impl_.reserve(n_objects);
    }

    //! Allocate memory for an object.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory for a new object
    //!  or NULL if memory can't be allocated.
    void* allocate() {
        return impl_.allocate();
    }

    //! Return memory to pool.
    void deallocate(void* memory) {
        impl_.deallocate(memory);
    }

    //! Destroy object and deallocate its memory.
    template <class T> void destroy_object(T& object) {
        object.~T();
        deallocate(&object);
    }

private:
    AlignedStorage<EmbeddedCapacity> embedded_data_;
    SlabPoolImpl impl_;
};

} // namespace core
} // namespace roc

//! Placement new for core::SlabPool.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
template <size_t Capacity>
inline void* operator new(size_t size, roc::core::SlabPool<Capacity>& pool) throw() {
    roc_panic_if_not(size <= pool.object_size());
    return pool.allocate();
}

//! Placement delete for core::SlabPool.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <size_t Capacity>
inline void operator delete(void* ptr, roc::core::SlabPool<Capacity>& pool) throw() {
    pool.deallocate(ptr);
}

#endif // ROC_CORE_SLAB_POOL_H_
