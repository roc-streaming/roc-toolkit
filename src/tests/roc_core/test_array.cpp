/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/heap_arena.h"

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

TEST_GROUP(array) {
    HeapArena arena;

    void setup() {
        Object::n_objects = 0;
    }
};

TEST(array, grow) {
    Array<Object, EmbeddedCap> array(arena);

    LONGS_EQUAL(0, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow(3));

    LONGS_EQUAL(3, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow(1));

    LONGS_EQUAL(3, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);
}

TEST(array, grow_exp) {
    Array<Object, EmbeddedCap> array(arena);

    LONGS_EQUAL(0, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow_exp(3));

    LONGS_EQUAL(4, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow_exp(1));

    LONGS_EQUAL(4, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow_exp(4));

    LONGS_EQUAL(4, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow_exp(5));

    LONGS_EQUAL(8, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);
}

TEST(array, resize) {
    Array<Object, EmbeddedCap> array(arena);

    LONGS_EQUAL(0, array.capacity());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.resize(3));

    LONGS_EQUAL(3, array.capacity());
    LONGS_EQUAL(3, array.size());
    LONGS_EQUAL(3, Object::n_objects);

    CHECK(array.resize(1));

    LONGS_EQUAL(3, array.capacity());
    LONGS_EQUAL(1, array.size());
    LONGS_EQUAL(1, Object::n_objects);
}

TEST(array, is_empty) {
    Array<Object, EmbeddedCap> array(arena);

    CHECK(array.size() == 0);
    CHECK(array.is_empty());

    CHECK(array.resize(1));

    CHECK(array.size() != 0);
    CHECK(!array.is_empty());

    CHECK(array.resize(0));

    CHECK(array.size() == 0);
    CHECK(array.is_empty());
}

TEST(array, push_back) {
    { // auto grow
        Array<Object, EmbeddedCap> array(arena);

        for (size_t n = 0; n < NumObjects; n++) {
            CHECK(array.push_back(Object(n)));

            CHECK(array.capacity() >= n + 1);
            LONGS_EQUAL(n + 1, array.size());
            LONGS_EQUAL(n + 1, Object::n_objects);
        }

        for (size_t n = 0; n < NumObjects; n++) {
            LONGS_EQUAL(n, array[n].value);
        }
    }
    { // explicit grow
        Array<Object, EmbeddedCap> array(arena);

        CHECK(array.grow(NumObjects));

        for (size_t n = 0; n < NumObjects; n++) {
            CHECK(array.push_back(Object(n)));

            LONGS_EQUAL(NumObjects, array.capacity());
            LONGS_EQUAL(n + 1, array.size());
            LONGS_EQUAL(n + 1, Object::n_objects);
        }

        for (size_t n = 0; n < NumObjects; n++) {
            LONGS_EQUAL(n, array[n].value);
        }
    }
}

TEST(array, pop_back) {
    Array<Object, EmbeddedCap> array(arena);

    CHECK(array.grow(NumObjects));

    for (size_t n = 0; n < NumObjects; n++) {
        CHECK(array.push_back(Object(n)));

        LONGS_EQUAL(NumObjects, array.capacity());
        LONGS_EQUAL(n + 1, array.size());
        LONGS_EQUAL(n + 1, Object::n_objects);
    }

    LONGS_EQUAL(NumObjects, array.size());
    for (size_t n = NumObjects; n >= 1; n--) {
        LONGS_EQUAL(n - 1, array[n - 1].value);
        array.pop_back();
        LONGS_EQUAL(n - 1, array.size());
    }

    LONGS_EQUAL(NumObjects, array.capacity());
    CHECK(array.is_empty());
}

TEST(array, data_pointer) {
    Array<Object, EmbeddedCap> array(arena);

    CHECK(array.resize(NumObjects));

    CHECK(array.data() != NULL);

    for (size_t n = 0; n < NumObjects; n++) {
        POINTERS_EQUAL(&array[n], array.data() + n);
    }
}

TEST(array, front) {
    Array<Object, EmbeddedCap> array(arena);

    CHECK(array.size() == 0);
    CHECK(array.is_empty());

    CHECK(array.push_back(Object(1)));
    LONGS_EQUAL(array.front().value, 1);

    CHECK(array.push_back(Object(2)));
    LONGS_EQUAL(array.front().value, 1);
}

TEST(array, back) {
    Array<Object, EmbeddedCap> array(arena);

    CHECK(array.size() == 0);
    CHECK(array.is_empty());

    CHECK(array.push_back(Object(1)));
    LONGS_EQUAL(array.back().value, 1);

    CHECK(array.push_back(Object(2)));
    LONGS_EQUAL(array.back().value, 2);
}

TEST(array, embedding) {
    Array<Object, EmbeddedCap> array(arena);

    CHECK(array.resize(EmbeddedCap));

    LONGS_EQUAL(0, arena.num_allocations());

    // data is inside of the array
    CHECK((char*)array.data() >= (char*)&array
          && (char*)(array.data() + EmbeddedCap) <= (char*)&array + sizeof(array));

    CHECK(array.resize(NumObjects));

    LONGS_EQUAL(1, arena.num_allocations());

    // data is outside of the array
    CHECK((char*)(array.data() + EmbeddedCap) < (char*)&array
          || (char*)array.data() > (char*)&array + sizeof(array));
}

TEST(array, constructor_destructor) {
    LONGS_EQUAL(0, arena.num_allocations());

    {
        Array<Object, EmbeddedCap> array(arena);

        CHECK(array.grow(3));

        CHECK(array.push_back(Object(1)));
        CHECK(array.push_back(Object(2)));
        CHECK(array.push_back(Object(3)));

        LONGS_EQUAL(0, arena.num_allocations());
        LONGS_EQUAL(3, Object::n_objects);

        CHECK(array.grow(15));

        LONGS_EQUAL(1, arena.num_allocations());
        LONGS_EQUAL(3, Object::n_objects);

        CHECK(array.push_back(Object(4)));
        CHECK(array.push_back(Object(5)));

        LONGS_EQUAL(1, arena.num_allocations());
        LONGS_EQUAL(5, Object::n_objects);
    }

    LONGS_EQUAL(0, arena.num_allocations());
    LONGS_EQUAL(0, Object::n_objects);
}

} // namespace core
} // namespace roc
