/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/slab_pool.h
//! @brief Memory pool.

#ifndef ROC_CORE_SLAB_POOL_H_
#define ROC_CORE_SLAB_POOL_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slab_pool_impl.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory pool guards.
enum SlabPoolGuard {
    //! Panic if leaks detected in pool destructor.
    SlabPool_LeakGuard = (1 << 0),
    //! Panic if detected buffer overflow when deallocating object.
    SlabPool_OverflowGuard = (1 << 1),
    //! Panic if detected ownership mismatch when deallocating object.
    SlabPool_OwnershipGuard = (1 << 2),
};

//! Default memory pool guards.
enum {
    SlabPool_DefaultGuards =
        (SlabPool_LeakGuard | SlabPool_OverflowGuard | SlabPool_OwnershipGuard)
};

//! Memory pool.
//!
//! Implements slab allocator algorithm. Allocates large chunks of memory ("slabs") from
//! given arena, and uses them for multiple smaller fixed-sized objects ("slots").
//!
//! Keeps track of free slots and uses them when possible. Automatically allocates new
//! slabs when there are no free slots available.
//!
//! Automatically grows size of new slabs exponentially. The user can also specify the
//! minimum and maximum limits for the slabs.
//!
//! The returned memory is always maximum-aligned.
//!
//! Implements three safety measures:
//!  - to catch double-free and other logical bugs, inserts link to owning pool before
//!    user data, and panics if it differs when memory is returned to pool
//!  - to catch buffer overflow bugs, inserts "canary guards" before and after user
//!    data, and panics if they are overwritten when memory is returned to pool
//!  - to catch uninitialized-access and use-after-free bugs, "poisons" memory when it
//!    returned to user, and when it returned back to the pool
//!
//! @tparam T defines pool object type. It is used to determine allocation size. If
//! runtime size is different from static size of T, it can be provided via constructor.
//!
//! @tparam EmbeddedCapacity defines number of slots embedded directly into SlabPool
//! instance. If non-zero, this memory will be used for first allocations, before
//! using memory arena.
//!
//! Thread-safe.
template <class T, size_t EmbeddedCapacity = 0>
class SlabPool : public IPool, public NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p name defines pool name, used for logging
    //!  - @p arena is used to allocate slabs
    //!  - @p object_size defines size of single object in bytes
    //!  - @p min_alloc_bytes defines minimum size in bytes per request to arena
    //!  - @p max_alloc_bytes defines maximum size in bytes per request to arena
    //!  - @p guards defines options to modify behaviour as indicated in SlabPoolGuard
    SlabPool(const char* name,
             IArena& arena,
             size_t object_size = sizeof(T),
             size_t min_alloc_bytes = 0,
             size_t max_alloc_bytes = 0,
             size_t guards = SlabPool_DefaultGuards)
        : impl_(name,
                arena,
                object_size,
                min_alloc_bytes,
                max_alloc_bytes,
                embedded_data_.memory(),
                embedded_data_.size(),
                guards) {
    }

    //! Get size of the allocation per object.
    virtual size_t allocation_size() const {
        return impl_.allocation_size();
    }

    //! Get size of the object.
    virtual size_t object_size() const {
        return impl_.object_size();
    }

    //! Reserve memory for given number of objects.
    virtual ROC_ATTR_NODISCARD bool reserve(size_t n_objects) {
        return impl_.reserve(n_objects);
    }

    //! Allocate memory for an object.
    virtual void* allocate() {
        return impl_.allocate();
    }

    //! Return memory to pool.
    virtual void deallocate(void* memory) {
        impl_.deallocate(memory);
    }

    //! Get number of guard failures detected.
    size_t num_guard_failures() const {
        return impl_.num_guard_failures();
    }

private:
    enum {
        SlotSize = (sizeof(SlabPoolImpl::SlotHeader) + sizeof(SlabPoolImpl::SlotCanary)
                    + sizeof(T) + sizeof(SlabPoolImpl::SlotCanary) + sizeof(AlignMax) - 1)
            / sizeof(AlignMax) * sizeof(AlignMax)
    };
    AlignedStorage<EmbeddedCapacity * SlotSize> embedded_data_;
    SlabPoolImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SLAB_POOL_H_
