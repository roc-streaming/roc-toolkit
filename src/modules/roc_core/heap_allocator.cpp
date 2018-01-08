/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/heap_allocator.h"

namespace roc {
namespace core {

HeapAllocator::~HeapAllocator() {
    if (num_allocations_ != 0) {
        roc_panic("heap allocator: detected leak, num_allocations=%d",
                  (int)num_allocations_);
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
