/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/ring_queue.h"

namespace roc {
namespace core {

namespace {

enum { NumObjects = 20, EmbeddedCap = 10 };

struct Object {
    static long n_objects;

    size_t value;

    explicit Object(size_t v = 0)
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

TEST_GROUP(ring_queue) {
    HeapArena arena;

    void setup() {
        Object::n_objects = 0;
    }
};

TEST(ring_queue, is_empty_is_full) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    CHECK(queue.is_empty());
    CHECK(!queue.is_full());

    queue.push_back(Object(0));
    CHECK(!queue.is_empty());
    CHECK(!queue.is_full());

    for (size_t n = 1; n < NumObjects; n++) {
        queue.push_back(Object(n));
    }
    CHECK(!queue.is_empty());
    CHECK(queue.is_full());

    queue.pop_front();
    CHECK(!queue.is_empty());
    CHECK(!queue.is_full());

    for (size_t n = 1; n < NumObjects; n++) {
        queue.pop_front();
    }
    CHECK(queue.is_empty());
    CHECK(!queue.is_full());
}

TEST(ring_queue, push_back) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    for (size_t n = 0; n < NumObjects; n++) {
        queue.push_back(Object(n));

        LONGS_EQUAL(n + 1, queue.size());
        LONGS_EQUAL(n + 1, Object::n_objects);

        LONGS_EQUAL(0, queue.front().value);
        LONGS_EQUAL(n, queue.back().value);
    }
}

TEST(ring_queue, push_front) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    for (size_t n = 0; n < NumObjects; n++) {
        queue.push_front(Object(n));

        LONGS_EQUAL(n + 1, queue.size());
        LONGS_EQUAL(n + 1, Object::n_objects);

        LONGS_EQUAL(n, queue.front().value);
        LONGS_EQUAL(0, queue.back().value);
    }
}

TEST(ring_queue, pop_back) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    for (size_t n = 0; n < NumObjects; n++) {
        queue.push_back(Object(n));
    }

    for (size_t n = 0; n < NumObjects; n++) {
        LONGS_EQUAL(0, queue.front().value);
        LONGS_EQUAL(NumObjects - n - 1, queue.back().value);

        LONGS_EQUAL(NumObjects - n, queue.size());
        queue.pop_back();
        LONGS_EQUAL(NumObjects - 1 - n, Object::n_objects);
    }
}

TEST(ring_queue, pop_front) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    for (size_t n = 0; n < NumObjects; n++) {
        queue.push_back(Object(n));
    }

    for (size_t n = 0; n < NumObjects; n++) {
        LONGS_EQUAL(n, queue.front().value);
        LONGS_EQUAL(NumObjects - 1, queue.back().value);

        LONGS_EQUAL(NumObjects - n, queue.size());
        queue.pop_front();
        LONGS_EQUAL(NumObjects - 1 - n, Object::n_objects);
    }
}

TEST(ring_queue, front_back) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    queue.push_back(Object(0));
    queue.push_back(Object(1));

    LONGS_EQUAL(0, queue.front().value);
    LONGS_EQUAL(1, queue.back().value);

    for (size_t n = 2; n < NumObjects; n++) {
        queue.push_back(Object(n));
    }
    LONGS_EQUAL(0, queue.front().value);
    LONGS_EQUAL(NumObjects - 1, queue.back().value);

    queue.pop_back();
    LONGS_EQUAL(0, queue.front().value);
    LONGS_EQUAL(NumObjects - 2, queue.back().value);

    queue.pop_front();
    LONGS_EQUAL(1, queue.front().value);
    LONGS_EQUAL(NumObjects - 2, queue.back().value);
}

TEST(ring_queue, wrap_around) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    for (size_t n = 0; n < NumObjects; n++) {
        queue.push_back(Object(n));
    }

    for (size_t n = 0; n < 5; n++) {
        queue.pop_front();
    }

    for (size_t n = 0; n < 5; n++) {
        queue.push_back(Object(NumObjects + n));
    }

    LONGS_EQUAL(NumObjects, queue.size());
    LONGS_EQUAL(5, queue.front().value);
    LONGS_EQUAL(NumObjects + 4, queue.back().value);
}

TEST(ring_queue, wrap_around_loop) {
    RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

    size_t head = 0;
    size_t tail = 0;

    for (size_t n = 0; n < NumObjects; n++) {
        queue.push_back(Object(tail));
        tail++;
    }

    for (int it = 0; it < NumObjects * 10; it++) {
        for (size_t n = 0; n < 5; n++) {
            queue.pop_front();
            head++;
        }

        for (size_t n = 0; n < 5; n++) {
            queue.push_back(Object(tail));
            tail++;
        }

        LONGS_EQUAL(NumObjects, queue.size());

        LONGS_EQUAL(head, queue.front().value);
        LONGS_EQUAL(tail - 1, queue.back().value);
    }
}

TEST(ring_queue, single_element) {
    RingQueue<Object, 1> queue(arena, 1);

    CHECK(queue.is_valid());
    LONGS_EQUAL(1, queue.capacity());
    LONGS_EQUAL(0, queue.size());
    CHECK(queue.is_empty());
    CHECK(!queue.is_full());

    // push_back
    queue.push_back(Object(42));
    LONGS_EQUAL(1, queue.size());
    CHECK(!queue.is_empty());
    CHECK(queue.is_full());
    LONGS_EQUAL(42, queue.front().value);
    LONGS_EQUAL(42, queue.back().value);

    // pop_front
    queue.pop_front();
    LONGS_EQUAL(0, queue.size());
    CHECK(queue.is_empty());
    CHECK(!queue.is_full());

    // push_front
    queue.push_front(Object(33));
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.is_full());
    LONGS_EQUAL(33, queue.front().value);
    LONGS_EQUAL(33, queue.back().value);

    LONGS_EQUAL(1, Object::n_objects);

    // pop_back
    queue.pop_back();
    CHECK(queue.is_empty());

    // Test behavior when empty
    CHECK(queue.is_empty());
    CHECK(!queue.is_full());
    LONGS_EQUAL(0, queue.size());

    LONGS_EQUAL(0, Object::n_objects);
}

TEST(ring_queue, embedding) {
    RingQueue<Object, EmbeddedCap> queue(arena, EmbeddedCap);

    CHECK(queue.is_valid());
    LONGS_EQUAL(EmbeddedCap, queue.capacity());
    LONGS_EQUAL(0, queue.size());
    LONGS_EQUAL(0, arena.num_allocations());

    // Fill the queue to capacity
    for (size_t n = 0; n < queue.capacity(); n++) {
        queue.push_back(Object(n));
        LONGS_EQUAL(n + 1, queue.size());
    }

    // Check that no allocations occurred
    LONGS_EQUAL(0, arena.num_allocations());

    // Check queue behavior at capacity
    LONGS_EQUAL(EmbeddedCap, queue.size());
    CHECK(queue.is_full());
    LONGS_EQUAL(0, queue.front().value);
    LONGS_EQUAL(EmbeddedCap - 1, queue.back().value);

    // Check wrapping behavior
    queue.pop_front();
    queue.push_back(Object(EmbeddedCap - 1));
    LONGS_EQUAL(EmbeddedCap, queue.size());
    LONGS_EQUAL(1, queue.front().value);
    LONGS_EQUAL(EmbeddedCap - 1, queue.back().value);

    // Check that no allocations occurred during the operations
    LONGS_EQUAL(0, arena.num_allocations());
}

TEST(ring_queue, constructor_destructor) {
    LONGS_EQUAL(0, arena.num_allocations());

    {
        RingQueue<Object, EmbeddedCap> queue(arena, NumObjects);

        CHECK(queue.is_valid());
        LONGS_EQUAL(NumObjects, queue.capacity());
        LONGS_EQUAL(0, queue.size());
        LONGS_EQUAL(0, Object::n_objects);
        LONGS_EQUAL(1, arena.num_allocations());
    }

    LONGS_EQUAL(0, arena.num_allocations());
    LONGS_EQUAL(0, Object::n_objects);
}

} // namespace core
} // namespace roc
