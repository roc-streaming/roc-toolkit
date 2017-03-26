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

#include "test_object.h"

namespace roc {
namespace test {

using namespace core;

enum { NumTestObjects = 5 };

typedef Array<TestObject, NumTestObjects> TestArray;

TEST_GROUP(array) {
    void* array_mem;
    TestArray* array_ptr;

    void setup() {
        TestObject::state.clear();

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
            LONGS_EQUAL(value, array().memory()[n].value());
        }
    }

    void expect_initialized(size_t from, size_t to) {
        expect_value(TestObject::Initialized, from, to);
    }

    void expect_uninitialized(size_t from, size_t to) {
        for (size_t n = from; n < to; n++) {
            CHECK(array().memory()[n].value() != TestObject::Initialized);
        }
    }
};

TEST(array, max_size) {
    LONGS_EQUAL(NumTestObjects, array().max_size());
}

TEST(array, empty) {
    LONGS_EQUAL(0, array().size());

    expect_uninitialized(0, NumTestObjects);
}

TEST(array, resize_grow) {
    array().resize(3);

    LONGS_EQUAL(3, array().size());

    expect_initialized(0, 3);

    expect_uninitialized(3, NumTestObjects);
}

TEST(array, resize_shrink) {
    array().resize(3);

    expect_initialized(0, 3);

    array().resize(1);

    LONGS_EQUAL(1, array().size());

    expect_initialized(0, 1);

    expect_uninitialized(1, NumTestObjects);
}

TEST(array, append) {
    for (size_t n = 0; n < NumTestObjects; n++) {
        array().append(TestObject(n));

        LONGS_EQUAL(n + 1, array().size());

        expect_uninitialized(n + 1, NumTestObjects);
    }

    for (size_t n = 0; n < NumTestObjects; n++) {
        LONGS_EQUAL(n, array()[n].value());
    }
}

TEST(array, memory) {
    array().resize(NumTestObjects);

    for (size_t n = 0; n < NumTestObjects; n++) {
        POINTERS_EQUAL(array().memory() + n, &array()[n]);
    }
}

TEST(array, allocate) {
    for (size_t n = 0; n < NumTestObjects; n++) {
        POINTERS_EQUAL(array().memory() + n, array().allocate());

        LONGS_EQUAL(n + 1, array().size());
    }

    expect_uninitialized(0, NumTestObjects);
}

TEST(array, constructor) {
    expect_uninitialized(0, NumTestObjects);

    new (array_mem) TestArray(NumTestObjects);

    expect_initialized(0, NumTestObjects);
}

TEST(array, destructor) {
    array().append(TestObject(11));
    array().append(TestObject(22));
    array().append(TestObject(33));

    array().~TestArray();

    expect_value(TestObject::Destroyed, 0, 3);

    expect_value(0, 3, NumTestObjects);
}

TEST(array, front_back) {
    for (size_t n = 0; n < NumTestObjects; n++) {
        array().append(TestObject(n));

        LONGS_EQUAL(0, array().front().value());
        LONGS_EQUAL(n, array().back().value());

        POINTERS_EQUAL(&array()[0], &array().front());
        POINTERS_EQUAL(&array()[n], &array().back());
    }
}

} // namespace test
} // namespace roc
