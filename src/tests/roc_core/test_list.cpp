/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/list.h"

namespace roc {
namespace test {

using namespace core;

namespace {

const size_t TEST_N_OBJECTS = 5;

struct Object : ListNode {};

} // namespace

TEST_GROUP(list) {
    Object objects[TEST_N_OBJECTS];

    List<Object, NoOwnership> list;
};

TEST(list, empty) {
    CHECK(list.front() == NULL);
    CHECK(list.back() == NULL);

    LONGS_EQUAL(0, list.size());
}

TEST(list, append_one) {
    list.append(objects[0]);

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[0], list.back());

    LONGS_EQUAL(1, list.size());
}

TEST(list, append_many) {
    for (size_t i = 0; i < TEST_N_OBJECTS; ++i) {
        LONGS_EQUAL(i, list.size());

        list.append(objects[i]);
    }

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[TEST_N_OBJECTS - 1], list.back());

    LONGS_EQUAL(TEST_N_OBJECTS, list.size());
}

TEST(list, iterate) {
    for (size_t i = 0; i < TEST_N_OBJECTS; ++i) {
        list.append(objects[i]);
    }

    int i = 0;
    for (Object* obj = list.front(); obj != NULL; obj = list.next(*obj)) {
        POINTERS_EQUAL(&objects[i++], obj);
    }
}

TEST(list, insert_front) {
    list.append(objects[0]);
    list.insert(objects[1], list.front());

    POINTERS_EQUAL(&objects[1], list.front());
    POINTERS_EQUAL(&objects[0], list.back());

    LONGS_EQUAL(2, list.size());
}

TEST(list, insert_middle) {
    list.append(objects[0]);
    list.append(objects[1]);

    list.insert(objects[2], &objects[1]);

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[1], list.back());

    LONGS_EQUAL(3, list.size());

    POINTERS_EQUAL(&objects[2], list.next(*list.front()));
}

TEST(list, insert_back) {
    list.append(objects[0]);
    list.append(objects[1]);

    list.insert(objects[2], NULL);

    LONGS_EQUAL(3, list.size());

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[2], list.back());
}

TEST(list, remove_front) {
    for (size_t i = 0; i < TEST_N_OBJECTS; ++i) {
        list.append(objects[i]);
    }

    for (size_t i = 0; i < TEST_N_OBJECTS; ++i) {
        LONGS_EQUAL(TEST_N_OBJECTS - i, list.size());

        list.remove(objects[i]);

        if (i != TEST_N_OBJECTS - 1) {
            POINTERS_EQUAL(&objects[i + 1], list.front());
            POINTERS_EQUAL(&objects[TEST_N_OBJECTS - 1], list.back());
        }
    }

    CHECK(list.front() == NULL);
    CHECK(list.back() == NULL);

    LONGS_EQUAL(0, list.size());
}

TEST(list, remove_middle) {
    list.append(objects[0]);
    list.append(objects[1]);
    list.append(objects[2]);

    LONGS_EQUAL(3, list.size());

    list.remove(objects[1]);

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[2], list.back());
    POINTERS_EQUAL(list.back(), list.next(*list.front()));

    LONGS_EQUAL(2, list.size());
}

} // namespace test
} // namespace roc
