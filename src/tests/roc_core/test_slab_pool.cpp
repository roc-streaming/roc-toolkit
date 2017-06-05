/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slab_pool.h"

namespace roc {
namespace test {

using namespace core;

namespace {

const size_t PoolSize = 5;

struct Object;

core::Array<Object*, PoolSize> destroyed;

struct Object : NonCopyable<> {
    ~Object() {
        destroyed.append(this);
    }
};

} // namespace

TEST_GROUP(slab_pool) {
    SlabPool<PoolSize, Object> pool;

    void setup() {
        destroyed.resize(0);
    }
};

TEST(slab_pool, empty) {
    LONGS_EQUAL(PoolSize, pool.available());
}

TEST(slab_pool, allocate_deallocate) {
    void* memory = pool.allocate();
    CHECK(memory);

    Object* obj = new (memory) Object;

    LONGS_EQUAL(PoolSize - 1, pool.available());
    LONGS_EQUAL(0, destroyed.size());

    pool.deallocate(obj);

    LONGS_EQUAL(PoolSize, pool.available());
    LONGS_EQUAL(0, destroyed.size());
}

TEST(slab_pool, new_destroy) {
    Object* obj = new (pool) Object;
    CHECK(obj);

    LONGS_EQUAL(PoolSize - 1, pool.available());
    LONGS_EQUAL(0, destroyed.size());

    pool.destroy(*obj);

    LONGS_EQUAL(PoolSize, pool.available());
    LONGS_EQUAL(1, destroyed.size());

    CHECK(destroyed.back() == obj);
}

TEST(slab_pool, new_destroy_all) {
    enum { NumIterations = 5 };

    for (size_t it = 0; it < NumIterations; it++) {
        Object* objects[PoolSize] = {};

        for (size_t n = 0; n < PoolSize; n++) {
            LONGS_EQUAL(PoolSize - n, pool.available());

            objects[n] = new (pool) Object;

            CHECK(objects[n]);
        }

        LONGS_EQUAL(0, pool.available());
        LONGS_EQUAL(0, destroyed.size());

        CHECK(new (pool) Object == NULL);

        for (size_t n = 0; n < PoolSize; n++) {
            LONGS_EQUAL(n, pool.available());
            LONGS_EQUAL(n, destroyed.size());

            pool.destroy(*objects[n]);

            CHECK(destroyed.back() == objects[n]);
        }

        LONGS_EQUAL(PoolSize, pool.available());
        LONGS_EQUAL(PoolSize, destroyed.size());

        destroyed.resize(0);
    }
}

} // namespace test
} // namespace roc
