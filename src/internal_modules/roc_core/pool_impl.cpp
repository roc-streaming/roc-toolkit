/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/pool_impl.h"
#include "roc_core/align_ops.h"
#include "roc_core/log.h"
#include "roc_core/memory_ops.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

namespace {

size_t clamp(size_t value, size_t lower_limit, size_t upper_limit) {
    if (value < lower_limit && lower_limit != 0) {
        value = lower_limit;
    }
    if (value > upper_limit && upper_limit != 0) {
        value = upper_limit;
    }
    return value;
}

} // namespace

PoolImpl::PoolImpl(const char* name,
                   IArena& arena,
                   size_t object_size,
                   size_t min_alloc_bytes,
                   size_t max_alloc_bytes,
                   void* preallocated_data,
                   size_t preallocated_size)
    : arena_(arena)
    , n_used_slots_(0)
    , slab_min_bytes_(clamp(min_alloc_bytes, preallocated_size, max_alloc_bytes))
    , slab_max_bytes_(max_alloc_bytes)
    , slot_size_no_boundary_(AlignOps::align_max(std::max(sizeof(Slot), object_size)))
    , slot_size_(BoundarySize + slot_size_no_boundary_ + BoundarySize)
    , slab_hdr_size_(AlignOps::align_max(sizeof(Slab)))
    , slab_cur_slots_(slab_min_bytes_ == 0 ? 1 : slots_per_slab_(slab_min_bytes_, true))
    , slab_max_slots_(slab_max_bytes_ == 0 ? 0 : slots_per_slab_(slab_max_bytes_, false))
    , object_size_(object_size) {
    roc_log(LogDebug,
            "pool: initializing:"
            " name=%s object_size=%lu min_slab=%luB(%luS) max_slab=%luB(%luS)",
            name, (unsigned long)slot_size_, (unsigned long)slab_min_bytes_,
            (unsigned long)slab_cur_slots_, (unsigned long)slab_max_bytes_,
            (unsigned long)slab_max_slots_);

    roc_panic_if_not(slab_cur_slots_ > 0);
    roc_panic_if_not(slab_cur_slots_ <= slab_max_slots_ || slab_max_slots_ == 0);

    if (preallocated_size > 0) {
        add_preallocated_memory_(preallocated_data, preallocated_size);
    }
}

PoolImpl::~PoolImpl() {
    deallocate_everything_();
}

size_t PoolImpl::object_size() const {
    return object_size_;
}

bool PoolImpl::reserve(size_t n_objects) {
    Mutex::Lock lock(mutex_);

    return reserve_slots_(n_objects);
}

void* PoolImpl::allocate() {
    Slot* slot;

    {
        Mutex::Lock lock(mutex_);

        slot = acquire_slot_();
    }

    if (slot == NULL) {
        return NULL;
    }

    return give_slot_to_user_(slot);
}

void PoolImpl::deallocate(void* memory) {
    if (memory == NULL) {
        roc_panic("pool: deallocating null pointer");
    }

    Slot* slot = take_slot_from_user_(memory);

    {
        Mutex::Lock lock(mutex_);

        release_slot_(slot);
    }
}

void* PoolImpl::give_slot_to_user_(Slot* slot) {
    slot->~Slot();

    void* canary_before = slot;
    void* memory = (char*)slot + BoundarySize;
    void* canary_after = (char*)slot + BoundarySize + slot_size_no_boundary_;

    MemoryOps::prepare_canary(canary_before, BoundarySize);
    MemoryOps::poison_before_use(memory, slot_size_no_boundary_);
    MemoryOps::prepare_canary(canary_after, BoundarySize);

    return memory;
}

PoolImpl::Slot* PoolImpl::take_slot_from_user_(void* memory) {
    void* canary_before = (char*)memory - BoundarySize;
    void* canary_after = (char*)memory + slot_size_no_boundary_;

    if (!MemoryOps::check_canary(canary_before, BoundarySize)) {
        roc_panic("pool: canary before object voilated");
    }
    MemoryOps::poison_after_use(memory, slot_size_no_boundary_);
    if (!MemoryOps::check_canary(canary_after, BoundarySize)) {
        roc_panic("pool: canary after object voilated");
    }

    return new (canary_before) Slot;
}

PoolImpl::Slot* PoolImpl::acquire_slot_() {
    if (free_slots_.is_empty()) {
        allocate_new_slab_();
    }

    Slot* slot = free_slots_.front();
    if (slot != NULL) {
        free_slots_.remove(*slot);
        n_used_slots_++;
    }

    return slot;
}

void PoolImpl::release_slot_(Slot* slot) {
    if (n_used_slots_ == 0) {
        roc_panic("pool: unpaired deallocation");
    }

    n_used_slots_--;
    free_slots_.push_front(*slot);
}

bool PoolImpl::reserve_slots_(size_t desired_slots) {
    if (desired_slots > free_slots_.size()) {
        increase_slab_size_(desired_slots - free_slots_.size());

        do {
            if (!allocate_new_slab_()) {
                return false;
            }
        } while (desired_slots > free_slots_.size());
    }

    return true;
}

void PoolImpl::increase_slab_size_(size_t desired_slots) {
    if (desired_slots > slab_max_slots_ && slab_max_slots_ != 0) {
        desired_slots = slab_max_slots_;
    }

    while (slab_cur_slots_ < desired_slots) {
        slab_cur_slots_ *= 2;

        if (slab_cur_slots_ > slab_max_slots_ && slab_max_slots_ != 0) {
            slab_cur_slots_ = slab_max_slots_;
            break;
        }
    }
}

bool PoolImpl::allocate_new_slab_() {
    const size_t slab_size_bytes = slot_offset_(slab_cur_slots_);

    void* memory = arena_.allocate(slab_size_bytes);
    if (memory == NULL) {
        return false;
    }

    Slab* slab = new (memory) Slab;
    slabs_.push_back(*slab);

    for (size_t n = 0; n < slab_cur_slots_; n++) {
        Slot* slot = new ((char*)slab + slot_offset_(n)) Slot;
        free_slots_.push_back(*slot);
    }

    increase_slab_size_(slab_cur_slots_ * 2);
    return true;
}

void PoolImpl::deallocate_everything_() {
    if (n_used_slots_ != 0) {
        roc_panic("pool: detected leak: used=%lu free=%lu", (unsigned long)n_used_slots_,
                  (unsigned long)free_slots_.size());
    }

    while (Slot* slot = free_slots_.front()) {
        free_slots_.remove(*slot);
    }

    while (Slab* slab = slabs_.front()) {
        slabs_.remove(*slab);
        arena_.deallocate(slab);
    }
}

void PoolImpl::add_preallocated_memory_(void* memory, size_t memory_size) {
    if (memory == NULL) {
        roc_panic("pool: preallocated memory is null");
    }

    const size_t n_slots = memory_size / slot_size_;

    for (size_t n = 0; n < n_slots; n++) {
        Slot* slot = new ((char*)memory + n * slot_size_) Slot;
        free_slots_.push_back(*slot);
    }
}

size_t PoolImpl::slots_per_slab_(size_t slab_size, bool round_up) const {
    roc_panic_if(slot_size_ == 0);

    if (slab_size < slab_hdr_size_) {
        return 1;
    }

    if (slab_size - slab_hdr_size_ < slot_size_) {
        return 1;
    }

    return ((slab_size - slab_hdr_size_) + (round_up ? (slot_size_ - 1) : 0))
        / slot_size_;
}

size_t PoolImpl::slot_offset_(size_t slot_index) const {
    return slab_hdr_size_ + slot_index * slot_size_;
}

} // namespace core
} // namespace roc
