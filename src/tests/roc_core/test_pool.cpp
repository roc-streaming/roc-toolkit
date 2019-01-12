/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/pool.h"

namespace roc {
namespace core {

namespace {

struct Object : NonCopyable<> {
    static long n_objects;

    char padding[1000];

    Object() {
        n_objects++;
    }

    ~Object() {
        n_objects--;
    }
};

long Object::n_objects = 0;

} // namespace

TEST_GROUP(pool) {
    HeapAllocator allocator;
};

TEST(pool, allocate_deallocate) {
    enum { ChunkSize = sizeof(Object) * 5 };

    Pool<Object> pool(allocator, sizeof(Object), ChunkSize, true);

    void* memory = pool.allocate();
    CHECK(memory);

    Object* object = new (memory) Object;

    LONGS_EQUAL(1, Object::n_objects);

    pool.deallocate(object);

    LONGS_EQUAL(1, Object::n_objects);
}

TEST(pool, new_destroy) {
    enum { ChunkSize = sizeof(Object) * 5 };

    Pool<Object> pool(allocator, sizeof(Object), ChunkSize, true);

    Object* object = new (pool) Object;

    LONGS_EQUAL(1, Object::n_objects);

    pool.destroy(*object);

    LONGS_EQUAL(0, Object::n_objects);
}

TEST(pool, new_destroy_many) {
    enum {
        NumChunks = 3,
        ObjectsPerChunk = 5,
        HeaderSize = 100,
        ChunkSize = sizeof(Object) * ObjectsPerChunk + HeaderSize
    };

    {
        Pool<Object> pool(allocator, sizeof(Object), ChunkSize, true);

        Object* objects[ObjectsPerChunk * NumChunks] = {};

        LONGS_EQUAL(0, allocator.num_allocations());
        LONGS_EQUAL(0, Object::n_objects);

        for (size_t n = 0; n < ObjectsPerChunk * NumChunks; n++) {
            objects[n] = new (pool) Object;
            CHECK(objects[n]);

            LONGS_EQUAL(n / ObjectsPerChunk + 1, allocator.num_allocations());
            LONGS_EQUAL(n + 1, Object::n_objects);
        }

        for (size_t n = 0; n < ObjectsPerChunk * NumChunks; n++) {
            pool.destroy(*objects[n]);

            LONGS_EQUAL(NumChunks, allocator.num_allocations());
            LONGS_EQUAL(ObjectsPerChunk * NumChunks - n - 1, Object::n_objects);
        }
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

} // namespace core
} // namespace roc
