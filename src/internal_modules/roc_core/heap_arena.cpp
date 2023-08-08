/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/heap_arena.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

int HeapArena::enable_leak_detection_ = false;

HeapArena::HeapArena()
    : num_allocations_(0) {
}

HeapArena::~HeapArena() {
    if (num_allocations_ != 0) {
        if (AtomicOps::load_seq_cst(enable_leak_detection_)) {
            roc_panic("heap arena: detected leak(s): %d objects was not freed",
                      (int)num_allocations_);
        }
    }
}

void HeapArena::enable_leak_detection() {
    AtomicOps::store_seq_cst(enable_leak_detection_, true);
}

size_t HeapArena::num_allocations() const {
    return (size_t)num_allocations_;
}

void* HeapArena::allocate(size_t size) {
    ++num_allocations_;
    return new char[size];
}

void HeapArena::deallocate(void* ptr) {
    if (!ptr) {
        roc_panic("heap arena: null pointer");
    }
    if (num_allocations_ <= 0) {
        roc_panic("heap arena: unpaired deallocate");
    }
    --num_allocations_;
    delete[](char*) ptr;
}

} // namespace core
} // namespace roc
