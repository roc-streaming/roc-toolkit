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

//! Slab pool.
//!
//! Allocates large slabs of memory ("slabs") from given allocator suitable to hold
//! multiple fixed-size objects ("slots").
//!
//! Keeps track of free slots and use them when possible. Automatically allocates new
//! slabs when necessary.
//!
//! The return memory is always maximum aligned. Thread-safe.
class SlabPool : public NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p allocator is used to allocate slabs
    //!  - @p object_size defines object size in bytes
    //!  - @p poison enables memory poisoning for debugging
    SlabPool(IAllocator& allocator, size_t object_size, bool poison);

    //! Deinitialize.
    ~SlabPool();

    //! Get size of objects in pool.
    size_t object_size() const;

    //! Allocate memory for an object.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory for a new object
    //!  or NULL if memory can't be allocated.
    void* allocate();

    //! Return memory to pool.
    void deallocate(void* memory);

private:
    enum { PoisonAllocated = 0x7a, PoisonDeallocated = 0x7d };

    struct Slab : ListNode { };
    struct Slot : ListNode { };

    Slot* get_slot_();
    void put_slot_(Slot* slot);

    void allocate_new_slab_();
    void deallocate_everything_();

    size_t slot_offset_(size_t n) const;

    Mutex mutex_;

    IAllocator& allocator_;

    List<Slab, NoOwnership> slabs_;
    List<Slot, NoOwnership> free_slots_;
    size_t n_used_slots_;

    const size_t slot_size_;
    const size_t slab_hdr_size_;
    size_t slab_n_slots_;

    const size_t object_size_;
    const bool poison_;
};

} // namespace core
} // namespace roc

//! Placement new for core::SlabPool.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
inline void* operator new(size_t size, roc::core::SlabPool& pool) throw() {
    roc_panic_if_not(size <= pool.object_size());
    return pool.allocate();
}

//! Placement delete for core::SlabPool.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
inline void operator delete(void* ptr, roc::core::SlabPool& pool) throw() {
    pool.deallocate(ptr);
}

#endif // ROC_CORE_SLAB_POOL_H_
