/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/heap_allocator.h"

namespace roc {
namespace core {

namespace {

enum { NumObjects = 5 };

struct Object {
    static long n_objects;

    size_t value;

    Object(size_t v = 0)
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
    HeapAllocator allocator;
};

TEST(array, empty) {
    Array<Object> array(allocator);

    LONGS_EQUAL(0, array.max_size());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);
}

TEST(array, grow) {
    Array<Object> array(allocator);

    CHECK(array.grow(3));

    LONGS_EQUAL(3, array.max_size());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow(1));

    LONGS_EQUAL(3, array.max_size());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);
}

TEST(array, grow_exp) {
    Array<Object> array(allocator);

    CHECK(array.grow_exp(3));

    LONGS_EQUAL(4, array.max_size());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow_exp(1));

    LONGS_EQUAL(4, array.max_size());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow_exp(4));

    LONGS_EQUAL(4, array.max_size());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);

    CHECK(array.grow_exp(5));

    LONGS_EQUAL(8, array.max_size());
    LONGS_EQUAL(0, array.size());
    LONGS_EQUAL(0, Object::n_objects);
}

TEST(array, resize) {
    Array<Object> array(allocator);

    CHECK(array.resize(3));

    LONGS_EQUAL(3, array.max_size());
    LONGS_EQUAL(3, array.size());
    LONGS_EQUAL(3, Object::n_objects);

    array.resize(1);

    LONGS_EQUAL(3, array.max_size());
    LONGS_EQUAL(1, array.size());
    LONGS_EQUAL(1, Object::n_objects);
}

TEST(array, push_back) {
    Array<Object> array(allocator);

    CHECK(array.grow(NumObjects));

    for (size_t n = 0; n < NumObjects; n++) {
        array.push_back(Object(n));

        LONGS_EQUAL(NumObjects, array.max_size());
        LONGS_EQUAL(n + 1, array.size());
        LONGS_EQUAL(n + 1, Object::n_objects);
    }

    for (size_t n = 0; n < NumObjects; n++) {
        LONGS_EQUAL(n, array[n].value);
    }
}

TEST(array, front_back) {
    Array<Object> array(allocator);

    CHECK(array.grow(NumObjects));

    for (size_t n = 0; n < NumObjects; n++) {
        array.push_back(Object(n));

        LONGS_EQUAL(0, array.front().value);
        LONGS_EQUAL(n, array.back().value);

        POINTERS_EQUAL(&array[0], &array.front());
        POINTERS_EQUAL(&array[n], &array.back());
    }
}

TEST(array, data) {
    Array<Object> array(allocator);

    CHECK(array.data() == NULL);

    CHECK(array.resize(NumObjects));

    CHECK(array.data() != NULL);

    for (size_t n = 0; n < NumObjects; n++) {
        POINTERS_EQUAL(&array[n], array.data() + n);
    }
}

TEST(array, constructor_destructor) {
    LONGS_EQUAL(0, allocator.num_allocations());

    {
        Array<Object> array(allocator);

        CHECK(array.grow(3));

        array.push_back(Object(1));
        array.push_back(Object(2));
        array.push_back(Object(3));

        LONGS_EQUAL(1, allocator.num_allocations());
        LONGS_EQUAL(3, Object::n_objects);

        CHECK(array.grow(5));

        array.push_back(Object(4));
        array.push_back(Object(5));

        LONGS_EQUAL(1, allocator.num_allocations());
        LONGS_EQUAL(5, Object::n_objects);
    }

    LONGS_EQUAL(0, allocator.num_allocations());
    LONGS_EQUAL(0, Object::n_objects);
}

} // namespace core
} // namespace roc
