/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/aligned_storage.h"
#include "roc_core/maybe.h"

namespace roc {
namespace test {

using namespace core;

namespace {

const size_t TEST_INITIALIZED = 111;
const size_t TEST_DESTROYED = 222;

struct Object {
    // Don't optimize-out value reads after destructor call (although it's UB).
    volatile size_t value;

    Object() {
        value = TEST_INITIALIZED;
    }

    ~Object() {
        CHECK(value == TEST_INITIALIZED);
        value = TEST_DESTROYED;
    }
};

typedef Maybe<Object> TestMaybe;

} // namespace

TEST_GROUP(maybe){};

TEST(maybe, empty) {
    TestMaybe maybe;

    CHECK(!maybe);
    CHECK(!maybe.get());
}

TEST(maybe, allocate) {
    TestMaybe maybe;

    Object* obj = new (maybe.allocate()) Object();

    CHECK(maybe);
    CHECK(maybe.get() == obj);
    CHECK(&*maybe == obj);
    CHECK(&maybe->value == &obj->value);

    CHECK(obj->value == TEST_INITIALIZED);

    CHECK(&TestMaybe::container_of(*obj) == &maybe);
}

TEST(maybe, placement_new) {
    TestMaybe maybe;

    Object* obj = new (maybe) Object();

    CHECK(maybe);
    CHECK(maybe.get() == obj);
}

TEST(maybe, destroy_allocated) {
    AlignedStorage<TestMaybe> storage;

    Object* obj;

    {
        TestMaybe* maybe = new (storage.mem()) TestMaybe;

        obj = new (maybe->allocate()) Object();

        CHECK(obj->value == TEST_INITIALIZED);

        maybe->~TestMaybe();
    }

    CHECK(obj->value == TEST_DESTROYED);
}

TEST(maybe, destroy_not_allocated) {
    AlignedStorage<TestMaybe> storage;

    Object* obj;

    {
        TestMaybe* maybe = new (storage.mem()) TestMaybe;

        obj = (Object*)maybe->memory();

        obj->value = TEST_INITIALIZED;

        maybe->~TestMaybe();
    }

    CHECK(obj->value == TEST_INITIALIZED);
}

} // namespace test
} // namespace roc
