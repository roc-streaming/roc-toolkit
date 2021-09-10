/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/heap_allocator.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

int HeapAllocator::panic_on_leak_;

void HeapAllocator::enable_panic_on_leak() {
    AtomicOps::store_seq_cst(panic_on_leak_, true);
}

HeapAllocator::HeapAllocator()
    : num_allocations_(0) {
}

HeapAllocator::~HeapAllocator() {
    if (num_allocations_ != 0) {
        if (AtomicOps::load_seq_cst(panic_on_leak_)) {
            roc_panic("heap allocator: detected leak(s): %d objects was not freed",
                      (int)num_allocations_);
        } else {
            roc_log(LogError,
                    "heap allocator: detected leak(s): %d objects was not freed",
                    (int)num_allocations_);
        }
    }
}

void* HeapAllocator::allocate(size_t size) {
    ++num_allocations_;
    return new char[size];
}

void HeapAllocator::deallocate(void* ptr) {
    if (num_allocations_ <= 0) {
        roc_panic("heap allocator: unpaired deallocate");
    }
    --num_allocations_;
    delete[](char*) ptr;
}

size_t HeapAllocator::num_allocations() const {
    return (size_t)num_allocations_;
}

} // namespace core
} // namespace roc
