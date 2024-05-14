/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/limited_pool.h"

namespace roc {
namespace core {

LimitedPool::LimitedPool(IPool& pool, MemoryLimiter& memory_limiter)
    : pool_(pool)
    , memory_limiter_(memory_limiter) {
}

size_t LimitedPool::allocation_size() const {
    return pool_.allocation_size();
}

size_t LimitedPool::object_size() const {
    return pool_.object_size();
}

ROC_ATTR_NODISCARD bool LimitedPool::reserve(size_t n_objects) {
    return pool_.reserve(n_objects);
}

void* LimitedPool::allocate() {
    size_t allocation_size = pool_.allocation_size();
    if (memory_limiter_.acquire(allocation_size)) {
        void* ptr = pool_.allocate();
        if (!ptr)
            memory_limiter_.release(allocation_size);
        return ptr;
    }
    return NULL;
}

void LimitedPool::deallocate(void* memory) {
    pool_.deallocate(memory);
    memory_limiter_.release(pool_.allocation_size());
}

} // namespace core
} // namespace roc
