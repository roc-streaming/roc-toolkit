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

TEST_GROUP(heap_arena) {};

TEST(heap_arena, guard_object) {
    HeapArena arena((DefaultHeapArenaFlags & ~HeapArenaFlag_EnableGuards));
    void* pointer = NULL;

    pointer = arena.allocate(127);
    CHECK(pointer);

    char* data = (char*)pointer;
    char* before_data = data - 1;
    char* after_data = data + 127;
    CHECK(*before_data == MemoryOps::Pattern_Canary);
    CHECK(*after_data == MemoryOps::Pattern_Canary);

    arena.deallocate(pointer);
}

TEST(heap_arena, guard_object_violations) {
    HeapArena arena((DefaultHeapArenaFlags & ~HeapArenaFlag_EnableGuards));

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

} // namespace core
} // namespace roc
