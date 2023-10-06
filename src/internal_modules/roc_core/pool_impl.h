/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/pool_impl.h
//! @brief Memory pool implementation class.

#ifndef ROC_CORE_POOL_IMPL_H_
#define ROC_CORE_POOL_IMPL_H_

#include "roc_core/align_ops.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory pool implementation class.
//!
//! This is non-template class that implements all pool logic, to avoid
//! polluting header file.
//!
//! @see Pool.
class PoolImpl : public NonCopyable<> {
public:
    //! Slot header.
    struct SlotHeader {
        //! The pool that the slot belongs to.
        PoolImpl* owner;
        //! Variable-length data surrounded by canary guard.
        AlignMax data[];
    };

    //! Canary guard which surrounds variable-length data.
    typedef AlignMax SlotCanary;

    //! Initialize.
    PoolImpl(const char* name,
             IArena& arena,
             size_t object_size,
             size_t min_alloc_bytes,
             size_t max_alloc_bytes,
             void* preallocated_data,
             size_t preallocated_size,
             size_t flags);

    //! Deinitialize.
    ~PoolImpl();

    //! Get size of objects in pool.
    size_t object_size() const;

    //! Reserve memory for given number of objects.
    ROC_ATTR_NODISCARD bool reserve(size_t n_objects);

    //! Allocate memory for an object.
    void* allocate();

    //! Return memory to pool.
    void deallocate(void* memory);

    //! Get number of guard failures.
    size_t num_guard_failures() const;

private:
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

    void add_preallocated_memory_(void* memory, size_t memory_size);

    size_t slots_per_slab_(size_t slab_size, bool round_up) const;
    size_t slot_offset_(size_t slot_index) const;

    Mutex mutex_;

    const char* name_;
    IArena& arena_;

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
    const size_t object_size_padding_;

    const size_t flags_;
    size_t num_guard_failures_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_POOL_IMPL_H_
