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
#include "roc_core/slab_pool.h"

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

} // namespace

TEST_GROUP(slab_pool) {
    enum { ObjectSize = 1000 };
};

TEST(slab_pool, object_size) {
    TestAllocator allocator;
    SlabPool pool(allocator, ObjectSize, true);

    LONGS_EQUAL(ObjectSize, pool.object_size());
}

TEST(slab_pool, allocate_deallocate) {
    TestAllocator allocator;

    {
        SlabPool pool(allocator, ObjectSize, true);

        LONGS_EQUAL(0, allocator.num_allocations());

        void* memory = pool.allocate();
        CHECK(memory);

        LONGS_EQUAL(1, allocator.num_allocations());

        pool.deallocate(memory);

        LONGS_EQUAL(1, allocator.num_allocations());
    }

    LONGS_EQUAL(0, allocator.num_allocations());
}

TEST(slab_pool, allocate_deallocate_many) {
    TestAllocator allocator;

    {
        SlabPool pool(allocator, ObjectSize, true);

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

TEST(slab_pool, reserve) {
    TestAllocator allocator;

    {
        SlabPool pool(allocator, ObjectSize, true);

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

TEST(slab_pool, reserve_many) {
    TestAllocator allocator;

    {
        SlabPool pool(allocator, ObjectSize, true);

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

TEST(slab_pool, min_size_allocate) {
    // min_size=0
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0);

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > ObjectSize);
        CHECK(allocator.cumulative_allocated_bytes < ObjectSize * 2);
    }
    // min_size=ObjectSize
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, ObjectSize);

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > ObjectSize);
        CHECK(allocator.cumulative_allocated_bytes < ObjectSize * 2);
    }
    // min_size=ObjectSize*2
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, ObjectSize * 2);

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > ObjectSize * 2);
        CHECK(allocator.cumulative_allocated_bytes < ObjectSize * 3);
    }
}

TEST(slab_pool, min_size_reserve) {
    // min_size=0
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0);

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > ObjectSize);
        CHECK(allocator.cumulative_allocated_bytes < ObjectSize * 2);
    }
    // min_size=ObjectSize
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, ObjectSize);

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > ObjectSize);
        CHECK(allocator.cumulative_allocated_bytes < ObjectSize * 2);
    }
    // min_size=ObjectSize*2
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, ObjectSize * 2);

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, allocator.num_allocations());

        CHECK(allocator.cumulative_allocated_bytes > ObjectSize * 2);
        CHECK(allocator.cumulative_allocated_bytes < ObjectSize * 3);
    }
}

TEST(slab_pool, max_size_allocate) {
    // max_size=0
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0, 0);

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
    // max_size=ObjectSize*100
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0, ObjectSize * 100);

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
    // max_size=ObjectSize*2
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0, ObjectSize * 2);

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

TEST(slab_pool, max_size_reserve) {
    // max_size=0
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0, 0);

        pool.reserve(10);

        LONGS_EQUAL(1, allocator.num_allocations());
    }
    // max_size=ObjectSize*100
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0, ObjectSize * 100);

        pool.reserve(10);

        LONGS_EQUAL(1, allocator.num_allocations());
    }
    // max_size=ObjectSize*2
    {
        TestAllocator allocator;
        SlabPool pool(allocator, ObjectSize, true, 0, ObjectSize * 2);

        pool.reserve(10);

        LONGS_EQUAL(10, allocator.num_allocations());
    }
}

} // namespace core
} // namespace roc
