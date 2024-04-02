/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/spsc_ring_buffer.h"

namespace roc {
namespace core {

namespace {

HeapArena arena;

struct Object {
    static long n_objects;

    int value;

    explicit Object(int v = 0)
        : value(v) {
        n_objects++;
    }

    Object(const Object& other)
        : value(other.value) {
        n_objects++;
    }

    ~Object() {
        n_objects--;
    }
};

long Object::n_objects = 0;

} // namespace

TEST_GROUP(spsc_ring_buffer) {};

TEST(spsc_ring_buffer, push_pop_one) {
    enum { BufSize = 10 };

    SpscRingBuffer<Object> sb(arena, BufSize);
    CHECK(sb.is_valid());

    CHECK(sb.is_empty());

    { // empty
        Object obj;
        CHECK(!sb.pop_front(obj));
    }

    CHECK(sb.is_empty());

    { // push
        Object obj(123);
        CHECK(sb.push_back(obj));
    }

    CHECK(!sb.is_empty());

    { // pop
        Object obj;
        CHECK(sb.pop_front(obj));
        LONGS_EQUAL(123, obj.value);
    }

    CHECK(sb.is_empty());

    { // empty
        Object obj;
        CHECK(!sb.pop_front(obj));
    }

    CHECK(sb.is_empty());
}

TEST(spsc_ring_buffer, push_pop_many) {
    enum { BufSize = 10, NumIters = 20 };

    SpscRingBuffer<Object> sb(arena, BufSize);
    CHECK(sb.is_valid());

    for (int iter = 0; iter < NumIters; iter++) {
        CHECK(sb.is_empty());

        for (int n = 0; n < BufSize; n++) {
            // push
            Object obj(n + 1);
            CHECK(sb.push_back(obj));
        }

        CHECK(!sb.is_empty());

        for (int n = 0; n < BufSize; n++) {
            // pop
            Object obj;
            CHECK(sb.pop_front(obj));
            LONGS_EQUAL(n + 1, obj.value);
        }

        CHECK(sb.is_empty());

        { // empty
            Object obj;
            CHECK(!sb.pop_front(obj));
        }

        CHECK(sb.is_empty());
    }
}

TEST(spsc_ring_buffer, ctor_dtor) {
    enum { BufSize = 10 };

    LONGS_EQUAL(0, Object::n_objects);

    {
        SpscRingBuffer<Object> sb(arena, BufSize);
        CHECK(sb.is_valid());

        LONGS_EQUAL(0, Object::n_objects);

        { // empty
            Object obj;
            CHECK(!sb.pop_front(obj));
        }

        LONGS_EQUAL(0, Object::n_objects);

        { // push
            Object obj1(11);
            Object obj2(22);
            Object obj3(33);

            LONGS_EQUAL(3, Object::n_objects);

            CHECK(sb.push_back(obj1));
            CHECK(sb.push_back(obj2));
            CHECK(sb.push_back(obj3));

            LONGS_EQUAL(6, Object::n_objects);
        }

        LONGS_EQUAL(3, Object::n_objects);

        { // pop
            Object obj;

            LONGS_EQUAL(4, Object::n_objects);

            CHECK(sb.pop_front(obj));
            LONGS_EQUAL(11, obj.value);

            LONGS_EQUAL(3, Object::n_objects);
        }

        LONGS_EQUAL(2, Object::n_objects);
    }

    LONGS_EQUAL(0, Object::n_objects);
}

TEST(spsc_ring_buffer, ctor_dtor_loop) {
    enum { BufSize = 10, NumIters = 20 };

    SpscRingBuffer<Object> sb(arena, BufSize);
    CHECK(sb.is_valid());

    for (int iter = 0; iter < NumIters; iter++) {
        LONGS_EQUAL(0, Object::n_objects);

        for (int n = 0; n < BufSize; n++) {
            // push
            Object obj(n + 1);
            CHECK(sb.push_back(obj));
        }

        LONGS_EQUAL(BufSize, Object::n_objects);

        { // overrun
            Object obj;
            CHECK(!sb.push_back(obj));
        }

        LONGS_EQUAL(BufSize, Object::n_objects);

        for (int n = 0; n < BufSize; n++) {
            // pop
            Object obj;
            CHECK(sb.pop_front(obj));
            LONGS_EQUAL(n + 1, obj.value);
        }

        LONGS_EQUAL(0, Object::n_objects);

        { // underrun
            Object obj;
            CHECK(!sb.pop_front(obj));
        }

        LONGS_EQUAL(0, Object::n_objects);
    }
}

} // namespace core
} // namespace roc
