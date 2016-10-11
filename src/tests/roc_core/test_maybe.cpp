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

#include "test_object.h"

namespace roc {
namespace test {

using namespace core;

typedef Maybe<TestObject> TestMaybe;

TEST_GROUP(maybe) {
    void setup() {
        TestObject::state.clear();
    }
};

TEST(maybe, empty) {
    TestMaybe maybe;

    CHECK(!maybe);
    CHECK(!maybe.get());
}

TEST(maybe, allocate) {
    TestMaybe maybe;

    TestObject* obj = new (maybe.allocate()) TestObject();

    CHECK(maybe);
    CHECK(maybe.get() == obj);
    CHECK(&*maybe == obj);

    CHECK(obj->value() == TestObject::Initialized);

    CHECK(&TestMaybe::container_of(*obj) == &maybe);
}

TEST(maybe, placement_new) {
    TestMaybe maybe;

    TestObject* obj = new (maybe) TestObject();

    CHECK(maybe);
    CHECK(maybe.get() == obj);
}

TEST(maybe, destroy_allocated) {
    AlignedStorage<TestMaybe> storage;

    TestObject* obj;

    {
        TestMaybe* maybe = new (storage.mem()) TestMaybe;

        obj = new (maybe->allocate()) TestObject();

        CHECK(obj->value() == TestObject::Initialized);

        maybe->~TestMaybe();
    }

    CHECK(obj->value() == TestObject::Destroyed);
}

TEST(maybe, destroy_not_allocated) {
    AlignedStorage<TestMaybe> storage;

    TestObject* obj;

    {
        TestMaybe* maybe = new (storage.mem()) TestMaybe;

        obj = (TestObject*)maybe->memory();

        obj->set_value(TestObject::Initialized);

        maybe->~TestMaybe();
    }

    CHECK(obj->value() == TestObject::Initialized);
}

} // namespace test
} // namespace roc
