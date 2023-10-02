/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/pool.h"

namespace roc {
namespace core {

namespace {

struct TestArena : public HeapArena {
    size_t cumulative_allocated_bytes;

    TestArena()
        : cumulative_allocated_bytes(0) {
    }

    virtual void* allocate(size_t size) {
        void* ptr = HeapArena::allocate(size);
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
    TestArena arena;
    Pool<TestObject> pool("test", arena);

    LONGS_EQUAL(sizeof(TestObject), pool.object_size());
}

TEST(pool, allocate_deallocate) {
    TestArena arena;

    {
        Pool<TestObject> pool("test", arena);

        LONGS_EQUAL(0, arena.num_allocations());

        void* memory = pool.allocate();
        CHECK(memory);

        LONGS_EQUAL(1, arena.num_allocations());

        pool.deallocate(memory);

        LONGS_EQUAL(1, arena.num_allocations());
    }

    LONGS_EQUAL(0, arena.num_allocations());
}

TEST(pool, allocate_deallocate_many) {
    TestArena arena;

    {
        Pool<TestObject> pool("test", arena);

        for (int i = 0; i < 10; i++) {
            void* pointers[1 + 2 + 4] = {};

            LONGS_EQUAL(i == 0 ? 0 : 3, arena.num_allocations());

            size_t n_pointers = 0;

            for (; n_pointers < 1; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 1 : 3, arena.num_allocations());

            for (; n_pointers < 1 + 2; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 2 : 3, arena.num_allocations());

            for (; n_pointers < 1 + 2 + 4; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
            }

            LONGS_EQUAL(3, arena.num_allocations());

            for (size_t n = 0; n < n_pointers; n++) {
                pool.deallocate(pointers[n]);
            }

            LONGS_EQUAL(3, arena.num_allocations());
        }
    }

    LONGS_EQUAL(0, arena.num_allocations());
}

TEST(pool, reserve) {
    TestArena arena;

    {
        Pool<TestObject> pool("test", arena);

        LONGS_EQUAL(0, arena.num_allocations());

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, arena.num_allocations());

        void* memory = pool.allocate();
        CHECK(memory);

        LONGS_EQUAL(1, arena.num_allocations());

        pool.deallocate(memory);

        LONGS_EQUAL(1, arena.num_allocations());
    }

    LONGS_EQUAL(0, arena.num_allocations());
}

TEST(pool, reserve_many) {
    TestArena arena;

    {
        Pool<TestObject> pool("test", arena);

        for (int i = 0; i < 10; i++) {
            void* pointers[1 + 2 + 4] = {};

            LONGS_EQUAL(i == 0 ? 0 : 3, arena.num_allocations());

            size_t n_pointers = 0;

            CHECK(pool.reserve(1));

            LONGS_EQUAL(i == 0 ? 1 : 3, arena.num_allocations());

            for (; n_pointers < 1; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 1 : 3, arena.num_allocations());

            CHECK(pool.reserve(2));

            LONGS_EQUAL(i == 0 ? 2 : 3, arena.num_allocations());

            for (; n_pointers < 1 + 2; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
                CHECK(pointers[n_pointers]);
            }

            LONGS_EQUAL(i == 0 ? 2 : 3, arena.num_allocations());

            CHECK(pool.reserve(4));

            LONGS_EQUAL(3, arena.num_allocations());

            for (; n_pointers < 1 + 2 + 4; n_pointers++) {
                pointers[n_pointers] = pool.allocate();
            }

            LONGS_EQUAL(3, arena.num_allocations());

            for (size_t n = 0; n < n_pointers; n++) {
                pool.deallocate(pointers[n]);
            }

            LONGS_EQUAL(3, arena.num_allocations());
        }
    }

    LONGS_EQUAL(0, arena.num_allocations());
}

TEST(pool, min_size_allocate) {
    // min_size=0
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject), // object_size
                              0,                  // min_size
                              0                   // max_size
        );

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, arena.num_allocations());

        CHECK(arena.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(arena.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject), // object_size
                              sizeof(TestObject), // min_size
                              0                   // max_size
        );

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, arena.num_allocations());

        CHECK(arena.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(arena.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)*2
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject),     // object_size
                              sizeof(TestObject) * 2, // min_size
                              0                       // max_size
        );

        void* mem = pool.allocate();
        CHECK(mem);
        pool.deallocate(mem);

        LONGS_EQUAL(1, arena.num_allocations());

        CHECK(arena.cumulative_allocated_bytes > sizeof(TestObject) * 2);
        CHECK(arena.cumulative_allocated_bytes < sizeof(TestObject) * 3);
    }
}

TEST(pool, min_size_reserve) {
    // min_size=0
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject), // object_size
                              0,                  // min_size
                              0                   // max_size
        );

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, arena.num_allocations());

        CHECK(arena.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(arena.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject), // object_size
                              sizeof(TestObject), // min_size
                              0                   // max_size
        );

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, arena.num_allocations());

        CHECK(arena.cumulative_allocated_bytes > sizeof(TestObject));
        CHECK(arena.cumulative_allocated_bytes < sizeof(TestObject) * 2);
    }
    // min_size=sizeof(TestObject)*2
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject),     // object_size
                              sizeof(TestObject) * 2, // min_size
                              0                       // max_size
        );

        CHECK(pool.reserve(1));

        LONGS_EQUAL(1, arena.num_allocations());

        CHECK(arena.cumulative_allocated_bytes > sizeof(TestObject) * 2);
        CHECK(arena.cumulative_allocated_bytes < sizeof(TestObject) * 3);
    }
}

TEST(pool, max_size_allocate) {
    // max_size=0
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
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

        LONGS_EQUAL(4, arena.num_allocations());
    }
    // max_size=sizeof(TestObject)*100
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
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

        LONGS_EQUAL(4, arena.num_allocations());
    }
    // max_size=sizeof(TestObject)*2
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
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

        LONGS_EQUAL(10, arena.num_allocations());
    }
}

TEST(pool, max_size_reserve) {
    // max_size=0
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject), // object_size
                              0,                  // min_size
                              0                   // max_size
        );

        CHECK(pool.reserve(10));

        LONGS_EQUAL(1, arena.num_allocations());
    }
    // max_size=sizeof(TestObject)*100
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject),      // object_size
                              0,                       // min_size
                              sizeof(TestObject) * 100 // max_size
        );

        CHECK(pool.reserve(10));

        LONGS_EQUAL(1, arena.num_allocations());
    }
    // max_size=sizeof(TestObject)*2
    {
        TestArena arena;
        Pool<TestObject> pool("test", arena,
                              sizeof(TestObject),    // object_size
                              0,                     // min_size
                              sizeof(TestObject) * 2 // max_size
        );

        CHECK(pool.reserve(10));

        LONGS_EQUAL(10, arena.num_allocations());
    }
}

TEST(pool, embedded_capacity) {
    TestArena arena;

    {
        Pool<TestObject, 5> pool("test", arena);

        LONGS_EQUAL(0, arena.num_allocations());

        void* pointers[10] = {};

        for (int n = 0; n < 5; n++) {
            pointers[n] = pool.allocate();
            CHECK(pointers[n]);
        }

        LONGS_EQUAL(0, arena.num_allocations());

        for (int n = 5; n < 10; n++) {
            pointers[n] = pool.allocate();
            CHECK(pointers[n]);
        }

        LONGS_EQUAL(1, arena.num_allocations());

        for (int n = 0; n < 10; n++) {
            pool.deallocate(pointers[n]);
        }
    }

    LONGS_EQUAL(0, arena.num_allocations());
}

TEST(pool, embedded_capacity_reuse) {
    TestArena arena;

    {
        Pool<TestObject, 5> pool("test", arena);

        for (int i = 0; i < 10; i++) {
            LONGS_EQUAL(0, arena.num_allocations());

            void* pointers[5] = {};

            for (int n = 0; n < 5; n++) {
                pointers[n] = pool.allocate();
                CHECK(pointers[n]);
            }

            LONGS_EQUAL(0, arena.num_allocations());

            for (int n = 0; n < 5; n++) {
                pool.deallocate(pointers[n]);
            }
        }
    }

    LONGS_EQUAL(0, arena.num_allocations());
}

TEST(pool, boundary_guard_around_object) {
    TestArena arena;
    Pool<TestObject, 1> pool("test", arena);
    void* pointer = NULL;

    pointer = pool.allocate();
    CHECK(pointer);

    char* data = (char*)pointer;
    char* before_data = data - 1;
    char* after_data = data + AlignOps::align_max(sizeof(TestObject));
    // See PoisonOps::Pattern_BoundaryGuard.
    char Pattern_BoundaryGuard = 0x7b;
    CHECK(*before_data == Pattern_BoundaryGuard);
    CHECK(*after_data == Pattern_BoundaryGuard);

    pool.deallocate(pointer);
}

IGNORE_TEST(pool, panics_on_boundary_guard_before_embedded_object_violation) {
    TestArena arena;
    Pool<TestObject, 1> pool("test", arena);
    void* pointer = NULL;

    pointer = pool.allocate();
    CHECK(pointer);

    char* data = (char*)pointer;
    data--;
    *data = 0x00;

    pool.deallocate(pointer);
}

IGNORE_TEST(pool, panics_on_boundary_guard_after_non_embedded_object_violation) {
    TestArena arena;
    Pool<TestObject, 1> pool("test", arena);
    void* pointers[2] = {};

    pointers[0] = pool.allocate();
    CHECK(pointers[0]);

    pointers[1] = pool.allocate();
    CHECK(pointers[1]);

    char* data = (char*)pointers[1];
    data += AlignOps::align_max(sizeof(TestObject));
    *data = 0x00;

    pool.deallocate(pointers[0]);
    pool.deallocate(pointers[1]);
}

} // namespace core
} // namespace roc
