/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/memory_ops.h"

namespace roc {
namespace core {

// clang-format off
TEST_GROUP(heap_arena) {
    void setup() {
        core::HeapArena::set_guards(0);
    }
    void teardown() {
        core::HeapArena::set_guards(core::HeapArena_DefaultGuards);
    }
};
// clang-format on

TEST(heap_arena, allocated_size) {
    HeapArena arena;
    void* pointer = NULL;

    CHECK(arena.compute_allocated_size(128) > 128);

    pointer = arena.allocate(128);
    CHECK(pointer);

    CHECK(arena.allocated_size(pointer) > 128);

    arena.deallocate(pointer);
}

TEST(heap_arena, guard_object) {
    HeapArena arena;
    void* pointer = NULL;

    pointer = arena.allocate(128);
    CHECK(pointer);

    char* data = (char*)pointer;
    char* before_data = data - 1;
    char* after_data = data + 128;
    CHECK(*before_data == MemoryOps::Pattern_Canary);
    CHECK(*after_data == MemoryOps::Pattern_Canary);

    arena.deallocate(pointer);
}

TEST(heap_arena, guard_object_violations) {
    HeapArena arena;

    void* pointers[2] = {};

    pointers[0] = arena.allocate(128);
    CHECK(pointers[0]);

    pointers[1] = arena.allocate(128);
    CHECK(pointers[1]);

    {
        char* data = (char*)pointers[0];
        data--;
        *data = 0x00;
    }
    arena.deallocate(pointers[0]);
    CHECK(arena.num_guard_failures() == 1);

    {
        char* data = (char*)pointers[1];
        data += 128;
        *data = 0x00;
    }
    arena.deallocate(pointers[1]);
    CHECK(arena.num_guard_failures() == 2);
}

TEST(heap_arena, ownership_guard) {
    HeapArena arena0;
    HeapArena arena1;

    void* pointer = arena0.allocate(128);
    CHECK(pointer);

    arena1.deallocate(pointer);
    CHECK(arena1.num_guard_failures() == 1);

    arena0.deallocate(pointer);
    CHECK(arena0.num_guard_failures() == 0);
}

} // namespace core
} // namespace roc
