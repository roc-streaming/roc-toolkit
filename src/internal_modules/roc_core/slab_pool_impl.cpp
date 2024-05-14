/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/slab_pool_impl.h"
#include "roc_core/align_ops.h"
#include "roc_core/log.h"
#include "roc_core/memory_ops.h"
#include "roc_core/panic.h"
#include "roc_core/slab_pool.h"

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

SlabPoolImpl::SlabPoolImpl(const char* name,
                           IArena& arena,
                           size_t object_size,
                           size_t min_alloc_bytes,
                           size_t max_alloc_bytes,
                           void* preallocated_data,
                           size_t preallocated_size,
                           size_t guards)
    : name_(name)
    , arena_(arena)
    , n_used_slots_(0)
    , slab_min_bytes_(clamp(min_alloc_bytes, preallocated_size, max_alloc_bytes))
    , slab_max_bytes_(max_alloc_bytes)
    , unaligned_slot_size_(sizeof(SlotHeader) + sizeof(SlotCanary) + object_size
                           + sizeof(SlotCanary))
    , slot_size_(AlignOps::align_max(std::max(sizeof(Slot), unaligned_slot_size_)))
    , slab_hdr_size_(AlignOps::align_max(sizeof(Slab)))
    , slab_cur_slots_(slab_min_bytes_ == 0 ? 1 : slots_per_slab_(slab_min_bytes_, true))
    , slab_max_slots_(slab_max_bytes_ == 0 ? 0 : slots_per_slab_(slab_max_bytes_, false))
    , object_size_(object_size)
    , object_size_padding_(slot_size_ - unaligned_slot_size_)
    , guards_(guards)
    , num_guard_failures_(0) {
    roc_panic_if_not(slab_cur_slots_ > 0);
    roc_panic_if_not(slab_cur_slots_ <= slab_max_slots_ || slab_max_slots_ == 0);

    if (preallocated_size > 0) {
        add_preallocated_memory_(preallocated_data, preallocated_size);
    }

    roc_log(LogDebug,
            "slab pool (%s): initializing:"
            " slot_size=%lu prealloc_size=%lu(%lu slots)"
            " min_slab=%lu(%lu slots) max_slab=%lu(%lu slots)",
            name_, (unsigned long)slot_size_, (unsigned long)preallocated_size,
            (unsigned long)free_slots_.size(), (unsigned long)slab_min_bytes_,
            (unsigned long)slab_cur_slots_, (unsigned long)slab_max_bytes_,
            (unsigned long)slab_max_slots_);
}

SlabPoolImpl::~SlabPoolImpl() {
    deallocate_everything_();
}

bool SlabPoolImpl::reserve(size_t n_objects) {
    Mutex::Lock lock(mutex_);

    return reserve_slots_(n_objects);
}

void* SlabPoolImpl::allocate() {
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

void SlabPoolImpl::deallocate(void* memory) {
    if (memory == NULL) {
        roc_panic("slab pool (%s): attempt to deallocate null pointer", name_);
    }

    Slot* slot = take_slot_from_user_(memory);

    if (slot == NULL) {
        return;
    }

    {
        Mutex::Lock lock(mutex_);

        release_slot_(slot);
    }
}

size_t SlabPoolImpl::allocation_size() const {
    // immutable, no lock
    return slot_size_;
}

size_t SlabPoolImpl::object_size() const {
    // immutable, no lock
    return object_size_;
}

size_t SlabPoolImpl::num_guard_failures() const {
    Mutex::Lock lock(mutex_);

    return num_guard_failures_;
}

void* SlabPoolImpl::give_slot_to_user_(Slot* slot) {
    slot->~Slot();

    SlotHeader* slot_hdr = (SlotHeader*)slot;

    slot_hdr->owner = this;

    void* canary_before = (char*)slot_hdr->data;
    void* memory = (char*)slot_hdr->data + sizeof(SlotCanary);
    void* canary_after = (char*)slot_hdr->data + sizeof(SlotCanary) + object_size_;

    MemoryOps::prepare_canary(canary_before, sizeof(SlotCanary));
    MemoryOps::poison_before_use(memory, object_size_);
    MemoryOps::prepare_canary(canary_after, object_size_padding_ + sizeof(SlotCanary));

    return memory;
}

SlabPoolImpl::Slot* SlabPoolImpl::take_slot_from_user_(void* memory) {
    SlotHeader* slot_hdr =
        ROC_CONTAINER_OF((char*)memory - sizeof(SlotCanary), SlotHeader, data);

    const bool is_owner = slot_hdr->owner == this;

    if (!is_owner) {
        if (report_guard_(SlabPool_OwnershipGuard)) {
            roc_panic(
                "slab pool (%s): attempt to deallocate slot not belonging to this pool:"
                " this_pool=%p slot_pool=%p",
                name_, (const void*)this, (const void*)slot_hdr->owner);
        }
        return NULL;
    }

    void* canary_before = (char*)slot_hdr->data;
    void* canary_after = (char*)slot_hdr->data + sizeof(SlotCanary) + object_size_;

    const bool canary_before_ok =
        MemoryOps::check_canary(canary_before, sizeof(SlotCanary));
    const bool canary_after_ok =
        MemoryOps::check_canary(canary_after, object_size_padding_ + sizeof(SlotCanary));

    if (!canary_before_ok || !canary_after_ok) {
        if (report_guard_(SlabPool_OverflowGuard)) {
            roc_panic("slab pool (%s): detected memory violation:"
                      " header_guard=%s footer_guard=%s",
                      name_, canary_before_ok ? "ok" : "corrupted",
                      canary_after_ok ? "ok" : "corrupted");
        }
    }

    MemoryOps::poison_after_use(memory, object_size_);

    return new (slot_hdr) Slot;
}

SlabPoolImpl::Slot* SlabPoolImpl::acquire_slot_() {
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

void SlabPoolImpl::release_slot_(Slot* slot) {
    if (n_used_slots_ == 0) {
        roc_panic("slab pool (%s): unpaired deallocation", name_);
    }

    n_used_slots_--;
    free_slots_.push_front(*slot);
}

bool SlabPoolImpl::reserve_slots_(size_t desired_slots) {
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

void SlabPoolImpl::increase_slab_size_(size_t desired_slots) {
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

bool SlabPoolImpl::allocate_new_slab_() {
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

void SlabPoolImpl::deallocate_everything_() {
    if (n_used_slots_ != 0) {
        if (report_guard_(SlabPool_LeakGuard)) {
            roc_panic("slab pool (%s): detected memory leak: n_used=%lu n_free=%lu",
                      name_, (unsigned long)n_used_slots_,
                      (unsigned long)free_slots_.size());
        }
    }

    while (Slot* slot = free_slots_.front()) {
        free_slots_.remove(*slot);
    }

    while (Slab* slab = slabs_.front()) {
        slabs_.remove(*slab);
        arena_.deallocate(slab);
    }
}

void SlabPoolImpl::add_preallocated_memory_(void* memory, size_t memory_size) {
    if (memory == NULL) {
        roc_panic("slab pool (%s): preallocated memory is null", name_);
    }

    const size_t n_slots = memory_size / slot_size_;

    for (size_t n = 0; n < n_slots; n++) {
        Slot* slot = new ((char*)memory + n * slot_size_) Slot;
        free_slots_.push_back(*slot);
    }
}

size_t SlabPoolImpl::slots_per_slab_(size_t slab_size, bool round_up) const {
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

size_t SlabPoolImpl::slot_offset_(size_t slot_index) const {
    return slab_hdr_size_ + slot_index * slot_size_;
}

bool SlabPoolImpl::report_guard_(size_t guard) const {
    num_guard_failures_++;
    return (guards_ & guard) != 0;
}

} // namespace core
} // namespace roc
