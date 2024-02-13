/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/limited_arena.h"

namespace roc {
namespace core {

// clang-format off
TEST_GROUP(limited_arena) {
    void setup() {
        core::HeapArena::set_guards(0);
    }
    void teardown() {
        core::HeapArena::set_guards(core::HeapArena_DefaultGuards);
    }
};
// clang-format on

TEST(limited_arena, enforce_limit) {
    HeapArena heap_arena;
    MemoryLimiter memory_limiter("test", 256);

    {
        LimitedArena arena(heap_arena, memory_limiter);
        void* pointer0 = arena.allocate(128);
        CHECK(pointer0);

        CHECK(memory_limiter.num_acquired() > 128);

        void* pointer1 = arena.allocate(128);
        CHECK(pointer1 == NULL);

        arena.deallocate(pointer0);

        pointer1 = arena.allocate(128);
        CHECK(pointer1);

        arena.deallocate(pointer1);

        CHECK(memory_limiter.num_acquired() == 0);
    }
}

TEST(limited_arena, allocated_size) {
    HeapArena heap_arena;
    MemoryLimiter memory_limiter("test", 256);

    {
        LimitedArena arena(heap_arena, memory_limiter);

        CHECK(arena.compute_allocated_size(128) > 128);

        void* pointer0 = arena.allocate(128);
        CHECK(pointer0);

        CHECK(arena.allocated_size(pointer0) > 128);

        arena.deallocate(pointer0);
    }
}
} // namespace core
} // namespace roc
