/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/slab_pool.h"

namespace roc {
namespace core {

SlabPool::SlabPool(IAllocator& allocator, size_t object_size, bool poison)
    : allocator_(allocator)
    , n_used_slots_(0)
    , slot_size_(max_align(std::max(sizeof(Slot), object_size)))
    , slab_hdr_size_(max_align(sizeof(Slab)))
    , slab_n_slots_(1)
    , object_size_(object_size)
    , poison_(poison) {
    roc_log(LogDebug, "slab pool: initializing: object_size=%lu poison=%d",
            (unsigned long)slot_size_, (int)poison);
}

SlabPool::~SlabPool() {
    deallocate_everything_();
}

size_t SlabPool::object_size() const {
    return object_size_;
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

void SlabPool::allocate_new_slab_() {
    void* memory = allocator_.allocate(slot_offset_(slab_n_slots_));
    if (memory == NULL) {
        return;
    }

    Slab* slab = new (memory) Slab;
    slabs_.push_back(*slab);

    for (size_t n = 0; n < slab_n_slots_; n++) {
        Slot* slot = new ((char*)slab + slot_offset_(n)) Slot;
        free_slots_.push_back(*slot);
    }

    slab_n_slots_ *= 2;
}

void SlabPool::deallocate_everything_() {
    if (n_used_slots_ != 0) {
        roc_panic("slab pool: detected leak: used=%lu free=%lu", (unsigned long)n_used_slots_,
                  (unsigned long)free_slots_.size());
    }

    while (Slot* slot = free_slots_.front()) {
        free_slots_.remove(*slot);
    }

    while (Slab* slab = slabs_.front()) {
        slabs_.remove(*slab);
        allocator_.deallocate(slab);
    }
}

size_t SlabPool::slot_offset_(size_t n) const {
    return slab_hdr_size_ + n * slot_size_;
}

} // namespace core
} // namespace roc
