/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/circular_buffer.h"

#include "test_object.h"

namespace roc {
namespace test {

using namespace core;

enum { NumTestObjects = 5 };

typedef CircularBuffer<TestObject, NumTestObjects> TestCircularBuffer;

TEST_GROUP(circular_buffer) {
    void* cb_mem;
    TestCircularBuffer* cb_ptr;

    void setup() {
        TestObject::state.clear();

        cb_mem = malloc(sizeof(TestCircularBuffer));
        memset(cb_mem, 0, sizeof(TestCircularBuffer));
        cb_ptr = new (cb_mem) TestCircularBuffer();
    }

    void teardown() {
        free(cb_mem);
    }

    TestCircularBuffer& cb() {
        return *cb_ptr;
    }

    void expect_uninitialized(size_t from, size_t to) {
        for (size_t n = from; n < to; n++) {
            CHECK(cb().memory()[n].value() != TestObject::Initialized);
        }
    }

    void expect_element(size_t offset, size_t index) {
        POINTERS_EQUAL(cb().memory() + offset, &cb()[index]);
    }
};

TEST(circular_buffer, max_size) {
    LONGS_EQUAL(NumTestObjects, cb().max_size());
}

TEST(circular_buffer, empty) {
    LONGS_EQUAL(0, cb().size());

    expect_uninitialized(0, NumTestObjects);
}

TEST(circular_buffer, push) {
    cb().push(TestObject(11));
    cb().push(TestObject(22));
    cb().push(TestObject(33));

    LONGS_EQUAL(3, cb().size());

    LONGS_EQUAL(11, cb().front().value());
    LONGS_EQUAL(33, cb().back().value());

    LONGS_EQUAL(11, cb()[0].value());
    LONGS_EQUAL(22, cb()[1].value());
    LONGS_EQUAL(33, cb()[2].value());

    expect_element(0, 0);
    expect_element(1, 1);
    expect_element(2, 2);
    expect_uninitialized(3, NumTestObjects);
}

TEST(circular_buffer, push_overwrite) {
    cb().push(TestObject(11));
    cb().push(TestObject(22));
    cb().push(TestObject(33));
    cb().push(TestObject(44));
    cb().push(TestObject(55));

    LONGS_EQUAL(NumTestObjects, cb().size());

    LONGS_EQUAL(11, cb().front().value());
    LONGS_EQUAL(55, cb().back().value());

    cb().push(TestObject(66));

    LONGS_EQUAL(NumTestObjects, cb().size());

    LONGS_EQUAL(22, cb().front().value());
    LONGS_EQUAL(66, cb().back().value());

    LONGS_EQUAL(22, cb()[0].value());
    LONGS_EQUAL(33, cb()[1].value());
    LONGS_EQUAL(44, cb()[2].value());
    LONGS_EQUAL(55, cb()[3].value());
    LONGS_EQUAL(66, cb()[4].value());

    expect_element(0, 4);
    expect_element(1, 0);
    expect_element(2, 1);
    expect_element(3, 2);
    expect_element(4, 3);
}

TEST(circular_buffer, shift) {
    cb().push(TestObject(11));
    cb().push(TestObject(22));
    cb().push(TestObject(33));

    LONGS_EQUAL(11, cb().shift().value());

    LONGS_EQUAL(2, cb().size());

    LONGS_EQUAL(22, cb().front().value());
    LONGS_EQUAL(33, cb().back().value());

    expect_uninitialized(0, 1);
    expect_element(1, 0);
    expect_element(2, 1);
    expect_uninitialized(3, NumTestObjects);
}

TEST(circular_buffer, push_shift_overwrite) {
    cb().push(TestObject(0));
    cb().push(TestObject(1));
    cb().push(TestObject(2));
    cb().push(TestObject(3));
    cb().push(TestObject(4));

    cb().shift();

    cb().push(TestObject(5));
    cb().push(TestObject(6));
    cb().push(TestObject(7));
    cb().push(TestObject(8));
    cb().push(TestObject(9));

    LONGS_EQUAL(NumTestObjects, cb().size());

    LONGS_EQUAL(5, cb().front().value());
    LONGS_EQUAL(9, cb().back().value());

    LONGS_EQUAL(5, cb()[0].value());
    LONGS_EQUAL(6, cb()[1].value());
    LONGS_EQUAL(7, cb()[2].value());
    LONGS_EQUAL(8, cb()[3].value());
    LONGS_EQUAL(9, cb()[4].value());

    expect_element(0, 0);
    expect_element(1, 1);
    expect_element(2, 2);
    expect_element(3, 3);
    expect_element(4, 4);

    LONGS_EQUAL(5, cb().shift().value());
    LONGS_EQUAL(6, cb().shift().value());
    LONGS_EQUAL(7, cb().shift().value());
    LONGS_EQUAL(8, cb().shift().value());
    LONGS_EQUAL(9, cb().shift().value());

    LONGS_EQUAL(0, cb().size());

    expect_uninitialized(0, NumTestObjects);
}

TEST(circular_buffer, rotate) {
    cb().push(TestObject(11));
    cb().push(TestObject(22));
    cb().push(TestObject(33));
    cb().push(TestObject(44));
    cb().push(TestObject(55));

    cb().rotate(2);

    LONGS_EQUAL(33, cb()[0].value());
    LONGS_EQUAL(44, cb()[1].value());
    LONGS_EQUAL(55, cb()[2].value());
    LONGS_EQUAL(11, cb()[3].value());
    LONGS_EQUAL(22, cb()[4].value());

    expect_element(0, 3);
    expect_element(1, 4);
    expect_element(2, 0);
    expect_element(3, 1);
    expect_element(4, 2);
}

TEST(circular_buffer, destructor) {
    cb().push(TestObject(11));
    cb().push(TestObject(22));
    cb().push(TestObject(33));
    cb().push(TestObject(44));
    cb().push(TestObject(55));
    cb().push(TestObject(66));

    cb().shift();
    cb().shift();
    cb().shift();

    LONGS_EQUAL(2, cb().size());

    LONGS_EQUAL(66, cb().memory()[0].value());
    LONGS_EQUAL(55, cb().memory()[4].value());

    cb().memory()[1].set_value(0);
    cb().memory()[2].set_value(0);
    cb().memory()[3].set_value(0);

    cb().~TestCircularBuffer();

    LONGS_EQUAL(TestObject::Destroyed, cb().memory()[0].value());
    LONGS_EQUAL(TestObject::Destroyed, cb().memory()[4].value());

    LONGS_EQUAL(0, cb().memory()[1].value());
    LONGS_EQUAL(0, cb().memory()[2].value());
    LONGS_EQUAL(0, cb().memory()[3].value());
}

TEST(circular_buffer, clear) {
    cb().push(TestObject(11));
    cb().push(TestObject(22));
    cb().push(TestObject(33));
    cb().push(TestObject(44));
    cb().push(TestObject(55));
    cb().push(TestObject(66));

    cb().clear();

    LONGS_EQUAL(0, cb().size());

    LONGS_EQUAL(TestObject::Destroyed, cb().memory()[0].value());
    LONGS_EQUAL(TestObject::Destroyed, cb().memory()[1].value());
    LONGS_EQUAL(TestObject::Destroyed, cb().memory()[2].value());
    LONGS_EQUAL(TestObject::Destroyed, cb().memory()[3].value());
    LONGS_EQUAL(TestObject::Destroyed, cb().memory()[4].value());

    cb().memory()[0].set_value(0);
    cb().memory()[1].set_value(0);
    cb().memory()[2].set_value(0);
    cb().memory()[3].set_value(0);
    cb().memory()[4].set_value(0);

    cb().~TestCircularBuffer();

    LONGS_EQUAL(0, cb().memory()[0].value());
    LONGS_EQUAL(0, cb().memory()[1].value());
    LONGS_EQUAL(0, cb().memory()[2].value());
    LONGS_EQUAL(0, cb().memory()[3].value());
    LONGS_EQUAL(0, cb().memory()[4].value());
}

} // namespace test
} // namespace roc
