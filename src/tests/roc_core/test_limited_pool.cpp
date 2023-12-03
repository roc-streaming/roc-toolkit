/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/limited_pool.h"
#include "roc_core/memory_limiter.h"
#include "roc_core/memory_ops.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slab_pool.h"

namespace roc {
namespace core {

namespace {

struct TestObject {
    char bytes[128];
};

} // namespace

TEST_GROUP(limited_pool) {};

TEST(limited_pool, enforce_limit) {
    HeapArena arena;
    SlabPool<TestObject> slabPool("test", arena);
    MemoryLimiter memory_limiter("test", 256);

    {
        LimitedPool pool(slabPool, memory_limiter);

        void* memory0 = pool.allocate();
        CHECK(memory0);
        CHECK(memory_limiter.num_acquired() > 128);

        void* memory1 = pool.allocate();
        CHECK(memory1 == NULL);

        pool.deallocate(memory0);

        memory1 = pool.allocate();
        CHECK(memory1);

        pool.deallocate(memory1);

        CHECK(memory_limiter.num_acquired() == 0);
    }
}

TEST(limited_pool, enforce_limit_despite_reserve) {
    HeapArena arena;
    SlabPool<TestObject> slabPool("test", arena);
    MemoryLimiter memory_limiter("test", 256);

    {
        LimitedPool pool(slabPool, memory_limiter);
        CHECK(pool.reserve(5));

        void* memory0 = pool.allocate();
        CHECK(memory0);
        CHECK(memory_limiter.num_acquired() > 128);

        void* memory1 = pool.allocate();
        CHECK(memory1 == NULL);

        pool.deallocate(memory0);

        memory1 = pool.allocate();
        CHECK(memory1);

        pool.deallocate(memory1);

        CHECK(memory_limiter.num_acquired() == 0);
    }
}

TEST(limited_pool, track_but_no_enforce_limit) {
    HeapArena arena;
    SlabPool<TestObject> slabPool("test", arena);
    MemoryLimiter memory_limiter("test", 0);

    {
        LimitedPool pool(slabPool, memory_limiter);

        void* memory0 = pool.allocate();
        CHECK(memory0);
        CHECK(memory_limiter.num_acquired() > 128);

        void* memory1 = pool.allocate();
        CHECK(memory1);
        CHECK(memory_limiter.num_acquired() > 256);

        pool.deallocate(memory0);
        pool.deallocate(memory1);
        CHECK(memory_limiter.num_acquired() == 0);
    }
}

TEST(limited_pool, allocation_size) {
    HeapArena arena;
    SlabPool<TestObject> slabPool("test", arena);
    MemoryLimiter memory_limiter("test", 256);

    {
        LimitedPool pool(slabPool, memory_limiter);

        CHECK(pool.allocation_size() > 128);
    }
}

} // namespace core
} // namespace roc
