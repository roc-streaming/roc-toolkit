/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/list.h"

namespace roc {
namespace core {

namespace {

enum { NumObjects = 5 };

struct Object : ListNode { };

} // namespace

TEST_GROUP(list_operations) {
    Object objects[NumObjects];

    List<Object, NoOwnership> list;
};

TEST(list_operations, empty) {
    CHECK(list.front() == NULL);
    CHECK(list.back() == NULL);

    LONGS_EQUAL(0, list.size());
}

TEST(list_operations, push_back_one) {
    list.push_back(objects[0]);

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[0], list.back());

    LONGS_EQUAL(1, list.size());
}

TEST(list_operations, push_back_many) {
    for (size_t i = 0; i < NumObjects; ++i) {
        LONGS_EQUAL(i, list.size());

        list.push_back(objects[i]);
    }

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[NumObjects - 1], list.back());

    LONGS_EQUAL(NumObjects, list.size());
}

TEST(list_operations, push_back_iterate) {
    for (size_t i = 0; i < NumObjects; ++i) {
        list.push_back(objects[i]);
    }

    int i = 0;
    for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
        POINTERS_EQUAL(&objects[i++], obj);
    }
}

TEST(list_operations, push_front_one) {
    list.push_front(objects[0]);

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[0], list.back());

    LONGS_EQUAL(1, list.size());
}

TEST(list_operations, push_front_many) {
    for (size_t i = 0; i < NumObjects; ++i) {
        LONGS_EQUAL(i, list.size());

        list.push_front(objects[i]);
    }

    POINTERS_EQUAL(&objects[NumObjects - 1], list.front());
    POINTERS_EQUAL(&objects[0], list.back());

    LONGS_EQUAL(NumObjects, list.size());
}

TEST(list_operations, push_front_iterate) {
    for (size_t i = 0; i < NumObjects; ++i) {
        list.push_front(objects[i]);
    }

    int i = NumObjects - 1;
    for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
        POINTERS_EQUAL(&objects[i--], obj);
    }
}

TEST(list_operations, insert_front) {
    list.push_back(objects[0]);
    list.insert_before(objects[1], *list.front());

    POINTERS_EQUAL(&objects[1], list.front());
    POINTERS_EQUAL(&objects[0], list.back());

    LONGS_EQUAL(2, list.size());
}

TEST(list_operations, insert_middle) {
    list.push_back(objects[0]);
    list.push_back(objects[1]);

    list.insert_before(objects[2], objects[1]);

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[1], list.back());

    LONGS_EQUAL(3, list.size());

    POINTERS_EQUAL(&objects[2], list.nextof(*list.front()));
}

TEST(list_operations, remove_front) {
    for (size_t i = 0; i < NumObjects; ++i) {
        list.push_back(objects[i]);
    }

    for (size_t i = 0; i < NumObjects; ++i) {
        LONGS_EQUAL(NumObjects - i, list.size());

        list.remove(objects[i]);

        if (i != NumObjects - 1) {
            POINTERS_EQUAL(&objects[i + 1], list.front());
            POINTERS_EQUAL(&objects[NumObjects - 1], list.back());
        }
    }

    CHECK(list.front() == NULL);
    CHECK(list.back() == NULL);

    LONGS_EQUAL(0, list.size());
}

TEST(list_operations, remove_middle) {
    list.push_back(objects[0]);
    list.push_back(objects[1]);
    list.push_back(objects[2]);

    LONGS_EQUAL(3, list.size());

    list.remove(objects[1]);

    POINTERS_EQUAL(&objects[0], list.front());
    POINTERS_EQUAL(&objects[2], list.back());
    POINTERS_EQUAL(list.back(), list.nextof(*list.front()));

    LONGS_EQUAL(2, list.size());
}

TEST(list_operations, contains) {
    CHECK(!list.contains(objects[0]));

    list.push_back(objects[0]);
    CHECK(list.contains(objects[0]));

    list.remove(objects[0]);
    CHECK(!list.contains(objects[0]));
}

} // namespace core
} // namespace roc
