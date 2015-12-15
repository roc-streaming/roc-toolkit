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

namespace roc {
namespace test {

using namespace core;

namespace {

const size_t TEST_INITIALIZED = 111;
const size_t TEST_DESTROYED = 222;

const size_t TEST_N_OBJECTS = 5;

struct Object {
    // Don't optimize-out value reads after destructor call (although it's UB).
    volatile size_t value;

    Object(size_t val = TEST_INITIALIZED)
        : value(val) {
    }

    ~Object() {
        value = TEST_DESTROYED;
    }
};

typedef Array<Object, TEST_N_OBJECTS> TestArray;

} // namespace

TEST_GROUP(array) {
    void* array_mem;
    TestArray* array_ptr;

    void setup() {
        array_mem = malloc(sizeof(TestArray));

        memset(array_mem, 0, sizeof(TestArray));

        array_ptr = new (array_mem) TestArray();
    }

    void teardown() {
        free(array_mem);
    }

    TestArray& array() {
        return *array_ptr;
    }

    void expect_value(size_t value, size_t from, size_t to) {
        for (size_t n = from; n < to; n++) {
            LONGS_EQUAL(value, array().memory()[n].value);
        }
    }

    void expect_initialized(size_t from, size_t to) {
        expect_value(TEST_INITIALIZED, from, to);
    }

    void expect_uninitialized(size_t from, size_t to) {
        for (size_t n = from; n < to; n++) {
            CHECK(array().memory()[n].value != TEST_INITIALIZED);
        }
    }
};

TEST(array, max_size) {
    LONGS_EQUAL(TEST_N_OBJECTS, array().max_size());
}

TEST(array, empty) {
    LONGS_EQUAL(0, array().size());

    expect_uninitialized(0, TEST_N_OBJECTS);
}

TEST(array, resize_grow) {
    array().resize(3);

    LONGS_EQUAL(3, array().size());

    expect_initialized(0, 3);

    expect_uninitialized(3, TEST_N_OBJECTS);
}

TEST(array, resize_shrink) {
    array().resize(3);

    expect_initialized(0, 3);

    array().resize(1);

    LONGS_EQUAL(1, array().size());

    expect_initialized(0, 1);

    expect_uninitialized(1, TEST_N_OBJECTS);
}

TEST(array, append) {
    for (size_t n = 0; n < TEST_N_OBJECTS; n++) {
        array().append(Object(n));

        LONGS_EQUAL(n + 1, array().size());

        expect_uninitialized(n + 1, TEST_N_OBJECTS);
    }

    for (size_t n = 0; n < TEST_N_OBJECTS; n++) {
        LONGS_EQUAL(n, array()[n].value);
    }
}

TEST(array, memory) {
    array().resize(TEST_N_OBJECTS);

    for (size_t n = 0; n < TEST_N_OBJECTS; n++) {
        POINTERS_EQUAL(array().memory() + n, &array()[n]);
    }
}

TEST(array, allocate) {
    for (size_t n = 0; n < TEST_N_OBJECTS; n++) {
        POINTERS_EQUAL(array().memory() + n, array().allocate());

        LONGS_EQUAL(n + 1, array().size());
    }

    expect_uninitialized(0, TEST_N_OBJECTS);
}

TEST(array, constructor) {
    expect_uninitialized(0, TEST_N_OBJECTS);

    new (array_mem) TestArray(TEST_N_OBJECTS);

    expect_initialized(0, TEST_N_OBJECTS);
}

TEST(array, destructor) {
    array().append(Object(11));
    array().append(Object(22));
    array().append(Object(33));

    array().~TestArray();

    expect_value(TEST_DESTROYED, 0, 3);

    expect_value(0, 3, TEST_N_OBJECTS);
}

TEST(array, front_back) {
    for (size_t n = 0; n < TEST_N_OBJECTS; n++) {
        array().append(Object(n));

        LONGS_EQUAL(0, array().front().value);
        LONGS_EQUAL(n, array().back().value);

        POINTERS_EQUAL(&array()[0], &array().front());
        POINTERS_EQUAL(&array()[n], &array().back());
    }
}

} // namespace test
} // namespace roc
