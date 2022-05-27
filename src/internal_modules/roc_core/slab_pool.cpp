/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/slab_pool.h"
#include "roc_core/align_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

SlabPool::SlabPool(IAllocator& allocator,
                   size_t object_size,
                   bool poison,
                   size_t min_alloc_bytes,
                   size_t max_alloc_bytes)
    : allocator_(allocator)
    , n_used_slots_(0)
    , slab_min_bytes_(min_alloc_bytes)
    , slab_max_bytes_(max_alloc_bytes == 0 ? 0
                                           : std::max(min_alloc_bytes, max_alloc_bytes))
    , slot_size_(AlignOps::align_max(std::max(sizeof(Slot), object_size)))
    , slab_hdr_size_(AlignOps::align_max(sizeof(Slab)))
    , slab_cur_slots_(slab_min_bytes_ == 0 ? 1 : slots_per_slab_(slab_min_bytes_, true))
    , slab_max_slots_(slab_max_bytes_ == 0 ? 0 : slots_per_slab_(slab_max_bytes_, false))
    , object_size_(object_size)
    , poison_(poison) {
    roc_log(LogDebug,
            "slab pool: initializing: object_size=%lu min_slab=%luB(%luS) "
            "max_slab=%luB(%luS) poison=%d",
            (unsigned long)slot_size_, (unsigned long)slab_min_bytes_,
            (unsigned long)slab_cur_slots_, (unsigned long)slab_max_bytes_,
            (unsigned long)slab_max_slots_, (int)poison);

    roc_panic_if_not(slab_cur_slots_ > 0);
    roc_panic_if_not(slab_cur_slots_ <= slab_max_slots_ || slab_max_slots_ == 0);
}

SlabPool::~SlabPool() {
    deallocate_everything_();
}

size_t SlabPool::object_size() const {
    return object_size_;
}

bool SlabPool::reserve(size_t n_objects) {
    return reserve_slots_(n_objects);
}

void* SlabPool::allocate() {
    Slot* slot = get_slot_();
    if (slot == NULL) {
        return NULL;
    }

    slot->~Slot();

    void* memory = slot;

    if (poison_) {
        memset(memory, PoisonAllocated, slot_size_);
    } else {
        memset(memory, 0, slot_size_);
    }

    return memory;
}

void SlabPool::deallocate(void* memory) {
    if (memory == NULL) {
        roc_panic("slab pool: deallocating null pointer");
    }

    if (poison_) {
        memset(memory, PoisonDeallocated, slot_size_);
    }

    Slot* slot = new (memory) Slot;
    put_slot_(slot);
}

bool SlabPool::reserve_slots_(size_t desired_slots) {
    Mutex::Lock lock(mutex_);

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

SlabPool::Slot* SlabPool::get_slot_() {
    Mutex::Lock lock(mutex_);

    if (free_slots_.size() == 0) {
        allocate_new_slab_();
    }

    Slot* slot = free_slots_.front();
    if (slot != NULL) {
        free_slots_.remove(*slot);
        n_used_slots_++;
    }

    return slot;
}

void SlabPool::put_slot_(Slot* slot) {
    Mutex::Lock lock(mutex_);

    if (n_used_slots_ == 0) {
        roc_panic("slab pool: unpaired deallocation");
    }

    n_used_slots_--;
    free_slots_.push_front(*slot);
}

void SlabPool::increase_slab_size_(size_t desired_slots) {
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

bool SlabPool::allocate_new_slab_() {
    const size_t slab_size_bytes = slot_offset_(slab_cur_slots_);

    void* memory = allocator_.allocate(slab_size_bytes);
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

void SlabPool::deallocate_everything_() {
    if (n_used_slots_ != 0) {
        roc_panic("slab pool: detected leak: used=%lu free=%lu",
                  (unsigned long)n_used_slots_, (unsigned long)free_slots_.size());
    }

    while (Slot* slot = free_slots_.front()) {
        free_slots_.remove(*slot);
    }

    while (Slab* slab = slabs_.front()) {
        slabs_.remove(*slab);
        allocator_.deallocate(slab);
    }
}

size_t SlabPool::slots_per_slab_(size_t slab_size, bool round_up) const {
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

size_t SlabPool::slot_offset_(size_t slot_index) const {
    return slab_hdr_size_ + slot_index * slot_size_;
}

} // namespace core
} // namespace roc
