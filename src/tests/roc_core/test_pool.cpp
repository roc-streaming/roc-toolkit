/*
 * Copyright (c) 2015 Roc Streaming authors
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

struct TestAllocator : public HeapAllocator {
    size_t cumulative_allocated_bytes;

    TestAllocator()
        : cumulative_allocated_bytes(0) {
    }

    virtual void* allocate(size_t size) {
        void* ptr = HeapAllocator::allocate(size);
        if (ptr) {
            cumulative_allocated_bytes += size;
        }

        return ptr;
    }
};

struct TestObject {
    char bytes[1000];
};

} // namespace

TEST_GROUP(pool) {};

TEST(pool, object_size) {
    TestAllocator allocator;
    Pool<TestObject> pool(allocator, true);

    LONGS_EQUAL(sizeof(TestObject), pool.object_size());
}

TEST(pool, allocate_deallocate) {
    TestAllocator allocator;

    {
        Pool<TestObject> pool(allocator, true);

        LONGS_EQUAL(0, allocator.num_allocations());

        void* memory = pool.allocate();
        CHECK(memory);

        LONGS_EQUAL(1, allocator.num_allocations());

        pool.deallocate(memory);

        LONGS_EQUAL(1, allocator.num_allocations());
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

TEST(pool, allocate_deallocate_many) {
    TestAllocator allocator;

    {
        Pool<TestObject> pool(allocator, true);

        for (int i = 0; i < 10; i++) {
            void* pointers[1 + 2 + 4] = {};

            LONGS_EQUAL(i == 0 ? 0 : 3, allocator.num_allocations());

            size_t n_pointers = 0;

            for (; n_pointers < 1; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 1 : 3, allocator.num_allocations());

            for (; n_pointers < 1 + 2; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 2 : 3, allocator.num_allocations());

            for (; n_pointers < 1 + 2 + 4; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
            }

            LONGS_EQUAL(3, allocator.num_allocations());

            for (size_t n = 0; n < n_pointers; n++) {
                pool.deallocate(pointers[n]);
            }

            LONGS_EQUAL(3, allocator.num_allocations());
        }
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

TEST(pool, reserve) {
    TestAllocator allocator;

    {
        Pool<TestObject> pool(allocator, true);

        LONGS_EQUAL(0, allocator.num_allocations());

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, allocator.num_allocations());

        void* memory = pool.allocate();
        CHECK(memory);

        LONGS_EQUAL(1, allocator.num_allocations());

        pool.deallocate(memory);

        LONGS_EQUAL(1, allocator.num_allocations());
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

TEST(pool, reserve_many) {
    TestAllocator allocator;

    {
        Pool<TestObject> pool(allocator, true);

        for (int i = 0; i < 10; i++) {
            void* pointers[1 + 2 + 4] = {};

            LONGS_EQUAL(i == 0 ? 0 : 3, allocator.num_allocations());

            size_t n_pointers = 0;

            CHECK(pool.reserve(1));

            LONGS_EQUAL(i == 0 ? 1 : 3, allocator.num_allocations());

            for (; n_pointers < 1; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 1 : 3, allocator.num_allocations());

            CHECK(pool.reserve(2));

            LONGS_EQUAL(i == 0 ? 2 : 3, allocator.num_allocations());

            for (; n_pointers < 1 + 2; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 2 : 3, allocator.num_allocations());

            CHECK(pool.reserve(4));

            LONGS_EQUAL(3, allocator.num_allocations());

            for (; n_pointers < 1 + 2 + 4; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
            }

            LONGS_EQUAL(3, allocator.num_allocations());

            for (size_t n = 0; n < n_pointers; n++) {
                pool.deallocate(pointers[n]);
            }

            LONGS_EQUAL(3, allocator.num_allocations());
        }
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

TEST(pool, min_size_allocate) {
    // min_size=0
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject), // object_size
                              0,                  // min_size
                              0                   // max_size
        );

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(allocator.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject), // object_size
                              sizeof(TestObject), // min_size
                              0                   // max_size
        );

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(allocator.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)*2
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject),     // object_size
                              sizeof(TestObject) * 2, // min_size
                              0                       // max_size
        );

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > sizeof(TestObject) * 2);
        CHECK(allocator.cumulative_allocated_bytes < sizeof(TestObject) * 3);
    }
}

TEST(pool, min_size_reserve) {
    // min_size=0
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject), // object_size
                              0,                  // min_size
                              0                   // max_size
        );

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(allocator.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject), // object_size
                              sizeof(TestObject), // min_size
                              0                   // max_size
        );

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(allocator.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)*2
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject),     // object_size
                              sizeof(TestObject) * 2, // min_size
                              0                       // max_size
        );

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > sizeof(TestObject) * 2);
        CHECK(allocator.cumulative_allocated_bytes < sizeof(TestObject) * 3);
    }
}

TEST(pool, max_size_allocate) {
    // max_size=0
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject), // object_size
                              0,                  // min_size
                              0                   // max_size
        );

        {
            void* pointers[10] = {};

            for (size_t i = 0; i < ROC_ARRAY_SIZE(pointers); i++) {
                pointers[i] = pool.allocate();
                CHECK(pointers[i]);
            }

            for (size_t i = 0; i < ROC_ARRAY_SIZE(pointers); i++) {
                pool.deallocate(pointers[i]);
            }
        }

        LONGS_EQUAL(4, allocator.num_allocations());
    }
    // max_size=sizeof(TestObject)*100
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject),      // object_size
                              0,                       // min_size
                              sizeof(TestObject) * 100 // max_size
        );

        {
            void* pointers[10] = {};

            for (size_t i = 0; i < ROC_ARRAY_SIZE(pointers); i++) {
                pointers[i] = pool.allocate();
                CHECK(pointers[i]);
            }

            for (size_t i = 0; i < ROC_ARRAY_SIZE(pointers); i++) {
                pool.deallocate(pointers[i]);
            }
        }

        LONGS_EQUAL(4, allocator.num_allocations());
    }
    // max_size=sizeof(TestObject)*2
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject),    // object_size
                              0,                     // min_size
                              sizeof(TestObject) * 2 // max_size
        );

        {
            void* pointers[10] = {};

            for (size_t i = 0; i < ROC_ARRAY_SIZE(pointers); i++) {
                pointers[i] = pool.allocate();
                CHECK(pointers[i]);
            }

            for (size_t i = 0; i < ROC_ARRAY_SIZE(pointers); i++) {
                pool.deallocate(pointers[i]);
            }
        }

        LONGS_EQUAL(10, allocator.num_allocations());
    }
}

TEST(pool, max_size_reserve) {
    // max_size=0
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject), // object_size
                              0,                  // min_size
                              0                   // max_size
        );

        CHECK(pool.reserve(10));

        LONGS_EQUAL(1, allocator.num_allocations());
    }
    // max_size=sizeof(TestObject)*100
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject),      // object_size
                              0,                       // min_size
                              sizeof(TestObject) * 100 // max_size
        );

        CHECK(pool.reserve(10));

        LONGS_EQUAL(1, allocator.num_allocations());
    }
    // max_size=sizeof(TestObject)*2
    {
        TestAllocator allocator;
        Pool<TestObject> pool(allocator, true,
                              sizeof(TestObject),    // object_size
                              0,                     // min_size
                              sizeof(TestObject) * 2 // max_size
        );

        CHECK(pool.reserve(10));

        LONGS_EQUAL(10, allocator.num_allocations());
    }
}

TEST(pool, embedded_capacity) {
    TestAllocator allocator;

    {
        Pool<TestObject, 5> pool(allocator, true);

        LONGS_EQUAL(0, allocator.num_allocations());

        void* pointers[10] = {};

        for (int n = 0; n < 5; n++) {
            pointers[n] = pool.allocate();
            CHECK(pointers[n]);
        }

        LONGS_EQUAL(0, allocator.num_allocations());

        for (int n = 5; n < 10; n++) {
            pointers[n] = pool.allocate();
            CHECK(pointers[n]);
        }

        LONGS_EQUAL(1, allocator.num_allocations());

        for (int n = 0; n < 10; n++) {
            pool.deallocate(pointers[n]);
        }
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

TEST(pool, embedded_capacity_reuse) {
    TestAllocator allocator;

    {
        Pool<TestObject, 5> pool(allocator, true);

        for (int i = 0; i < 10; i++) {
            LONGS_EQUAL(0, allocator.num_allocations());

            void* pointers[5] = {};

            for (int n = 0; n < 5; n++) {
                pointers[n] = pool.allocate();
                CHECK(pointers[n]);
            }

            LONGS_EQUAL(0, allocator.num_allocations());

            for (int n = 0; n < 5; n++) {
                pool.deallocate(pointers[n]);
            }
        }
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

} // namespace core
} // namespace roc
