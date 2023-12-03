/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/limited_arena.h"

namespace roc {
namespace core {

LimitedArena::LimitedArena(IArena& arena, MemoryLimiter& memory_limiter)
    : arena_(arena)
    , memory_limiter_(memory_limiter) {
}

void* LimitedArena::allocate(size_t size) {
    size_t allocated_size = arena_.compute_allocated_size(size);
    if (memory_limiter_.acquire(allocated_size)) {
        void* ptr = arena_.allocate(size);
        if (!ptr)
            memory_limiter_.release(allocated_size);
        return ptr;
    }
    return NULL;
}

void LimitedArena::deallocate(void* ptr) {
    size_t allocated_size = arena_.allocated_size(ptr);
    arena_.deallocate(ptr);
    memory_limiter_.release(allocated_size);
}

size_t LimitedArena::compute_allocated_size(size_t size) const {
    return arena_.compute_allocated_size(size);
}

size_t LimitedArena::allocated_size(void* ptr) const {
    return arena_.allocated_size(ptr);
}

} // namespace core
} // namespace roc
