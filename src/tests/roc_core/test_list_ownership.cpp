/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/list.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

namespace {

struct NoAllocation {
    template <class T> void destroy(T&) {
    }
};

struct Object : RefCounted<Object, NoAllocation>, ListNode {};

typedef List<Object, RefCountedOwnership> TestList;

} // namespace

TEST_GROUP(list_ownership) {};

TEST(list_ownership, pop_front) {
    Object obj1;
    Object obj2;

    TestList list;

    list.push_back(obj1);
    list.push_back(obj2);
    LONGS_EQUAL(1, obj1.getref());
    LONGS_EQUAL(1, obj2.getref());

    list.pop_front();
    LONGS_EQUAL(0, obj1.getref());
    POINTERS_EQUAL(&obj2, list.front().get());

    list.pop_front();
    LONGS_EQUAL(0, obj2.getref());
}

TEST(list_ownership, pop_back) {
    Object obj1;
    Object obj2;

    TestList list;

    list.push_back(obj1);
    list.push_back(obj2);
    LONGS_EQUAL(1, obj1.getref());
    LONGS_EQUAL(1, obj2.getref());

    list.pop_back();
    LONGS_EQUAL(0, obj2.getref());
    POINTERS_EQUAL(&obj1, list.back().get());

    list.pop_back();
    LONGS_EQUAL(0, obj1.getref());
}

TEST(list_ownership, push_back) {
    Object obj;

    TestList list;

    LONGS_EQUAL(0, obj.getref());

    list.push_back(obj);

    LONGS_EQUAL(1, obj.getref());
}

TEST(list_ownership, push_front) {
    Object obj;

    TestList list;

    LONGS_EQUAL(0, obj.getref());

    list.push_front(obj);

    LONGS_EQUAL(1, obj.getref());
}

TEST(list_ownership, insert) {
    Object obj1;
    Object obj2;

    TestList list;

    list.push_back(obj1);
    list.insert_before(obj2, obj1);

    LONGS_EQUAL(1, obj1.getref());
    LONGS_EQUAL(1, obj2.getref());
}

TEST(list_ownership, remove) {
    Object obj;

    TestList list;

    list.push_back(obj);

    LONGS_EQUAL(1, obj.getref());

    list.remove(obj);

    LONGS_EQUAL(0, obj.getref());
}

TEST(list_ownership, destructor) {
    Object obj;

    {
        TestList list;

        list.push_back(obj);

        LONGS_EQUAL(1, obj.getref());
    }

    LONGS_EQUAL(0, obj.getref());
}

TEST(list_ownership, pointers) {
    Object obj;

    TestList list;

    list.push_back(obj);

    POINTERS_EQUAL(&obj, list.front().get());
    POINTERS_EQUAL(&obj, list.back().get());

    LONGS_EQUAL(2, list.front()->getref());
    LONGS_EQUAL(2, list.back()->getref());
}

} // namespace core
} // namespace roc
