/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/free_list.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

namespace {

enum { NumObjects = 5 };

struct Object : FreeListNode<> {};

struct RefObject : RefCounted<RefObject, NoopAllocation>, FreeListNode<> {};

} // namespace

TEST_GROUP(free_list) {};

TEST(free_list, empty_list) {
    FreeList<Object, NoOwnership> list;
    CHECK(list.is_empty());
}

TEST(free_list, push_front) {
    {
        // push one element
        Object objects[NumObjects];
        FreeList<Object, NoOwnership> list;

        list.push_front(objects[0]);
        CHECK(!list.is_empty());
    }
    { // push many elements
        Object objects[NumObjects];
        FreeList<Object, NoOwnership> list;

        for (size_t i = 0; i < NumObjects; ++i) {
            list.push_front(objects[i]);
        }

        CHECK(!list.is_empty());
    }
}

TEST(free_list, pop_front) {
    // with push_front
    Object objects[NumObjects];
    FreeList<Object, NoOwnership> list;
    size_t size = 0;
    for (size_t i = 0; i < NumObjects; ++i) {
        list.push_front(objects[i]);
        size++;
    }

    for (size_t i = 0; i < NumObjects; ++i) {
        LONGS_EQUAL(NumObjects - i, size);
        Object* obj = list.pop_front();
        POINTERS_EQUAL(&objects[NumObjects - i - 1], obj);
        size--;
    }

    CHECK(list.is_empty());
}

TEST(free_list, iteration) {
    Object objects[NumObjects];
    FreeList<Object, NoOwnership> list;

    for (size_t i = 0; i < NumObjects; ++i) {
        list.push_front(objects[i]);
    }
}

TEST(free_list, ownership_operations) {
    { // push_front
        RefObject obj;
        FreeList<RefObject, RefCountedOwnership> list;

        LONGS_EQUAL(0, obj.getref());
        list.push_front(obj);
        LONGS_EQUAL(1, obj.getref());
    }
    { // pop_front
        RefObject obj1;
        RefObject obj2;

        FreeList<RefObject, RefCountedOwnership> list;

        list.push_front(obj1);
        list.push_front(obj2);
        LONGS_EQUAL(1, obj1.getref());
        LONGS_EQUAL(1, obj2.getref());

        {
            SharedPtr<RefObject> obj2_a = list.pop_front();
            LONGS_EQUAL(1, obj2.getref());
            POINTERS_EQUAL(&obj2, obj2_a.get());
        }
        LONGS_EQUAL(0, obj2.getref());

        {
            SharedPtr<RefObject> obj1_a = list.pop_front();
            LONGS_EQUAL(1, obj1.getref());
            POINTERS_EQUAL(&obj1, obj1_a.get());
        }
        LONGS_EQUAL(0, obj1.getref());
    }
}

TEST(free_list, ownership_destructor) {
    RefObject obj;
    {
        FreeList<RefObject, RefCountedOwnership> list;

        list.push_front(obj);

        LONGS_EQUAL(1, obj.getref());
    }

    LONGS_EQUAL(0, obj.getref());
}

} // namespace core

} // namespace roc
