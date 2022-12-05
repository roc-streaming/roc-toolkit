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

#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Slab pool.
//!
//! Allocates large chunks of memory ("slabs") from given allocator suitable to hold
//! multiple fixed-size objects ("slots").
//!
//! Keeps track of free slots and use them when possible. Automatically allocates new
//! slabs when there are no free slots.
//!
//! Automatically grows size of new slabs exponentially. The user can also specify the
//! minimum and maximum limits for the slab.
//!
//! The return memory is always maximum aligned. Thread-safe.
class SlabPool : public NonCopyable<> {
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
             size_t max_alloc_bytes = 0);

    //! Deinitialize.
    ~SlabPool();

    //! Get size of objects in pool.
    size_t object_size() const;

    //! Reserve memory for given number of objects.
    //! @returns
    //!  false if allocation failed.
    bool reserve(size_t n_objects);

    //! Allocate memory for an object.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory for a new object
    //!  or NULL if memory can't be allocated.
    void* allocate();

    //! Return memory to pool.
    void deallocate(void* memory);

    //! Destroy object and deallocate its memory.
    template <class T> void destroy_object(T& object) {
        object.~T();
        deallocate(&object);
    }

private:
    // Some good fillers for unused memory.
    // If we fill memory with these values and interpret it as 16-bit or 32-bit
    // integers, or as floats, the values will be rather high and will sound
    // loudly when trying to play them on sound card.
    enum { PoisonAllocated = 0x7a, PoisonDeallocated = 0x7d };

    struct Slab : ListNode {};
    struct Slot : ListNode {};

    void* give_slot_to_user_(Slot* slot);
    Slot* take_slot_from_user_(void* memory);

    Slot* acquire_slot_();
    void release_slot_(Slot* slot);
    bool reserve_slots_(size_t desired_slots);

    void increase_slab_size_(size_t desired_n_slots);
    bool allocate_new_slab_();
    void deallocate_everything_();

    size_t slots_per_slab_(size_t slab_size, bool round_up) const;
    size_t slot_offset_(size_t slot_index) const;

    Mutex mutex_;

    IAllocator& allocator_;

    List<Slab, NoOwnership> slabs_;
    List<Slot, NoOwnership> free_slots_;
    size_t n_used_slots_;

    const size_t slab_min_bytes_;
    const size_t slab_max_bytes_;

    const size_t slot_size_;
    const size_t slab_hdr_size_;

    size_t slab_cur_slots_;
    const size_t slab_max_slots_;

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
