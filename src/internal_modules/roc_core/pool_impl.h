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
private:
    struct Slab : ListNode {};
    struct Slot : ListNode {};

    // Slot layout when supplying data to user.
    struct UserSlot {
        PoolImpl* owner;
        AlignMax canary_before;
        AlignMax data[];
        // Another canary immediately after actual data. Any padding for data is
        // considered part of canary.
    };

public:
    //! Size for canary guard.
    enum { CanarySize = sizeof(AlignMax) };

    //! Size of slot given object size.
    //! Used at compile time to calculate embedded storage in main Pool class.
    template <size_t ObjectSize> struct SlotSize {
    private:
        enum {
            slot_size_ = MaxAlignedSize<sizeof(Slot)>::value,
            user_slot_size_ = MaxAlignedSize<sizeof(UserSlot)>::value
                + MaxAlignedSize<ObjectSize>::value + PoolImpl::CanarySize,
        };

    public:
        enum {
            value = user_slot_size_ > slot_size_ ? user_slot_size_ : slot_size_,
        };
    };

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

    //! Get number of buffer overflows detected.
    size_t num_buffer_overflows() const;

    //! Get number of invalid ownerships detected.
    size_t num_invalid_ownerships() const;

private:
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
    size_t num_buffer_overflows_;
    size_t num_invalid_ownerships_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_POOL_IMPL_H_
