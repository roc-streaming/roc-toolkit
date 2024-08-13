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

enum { NumObjects = 5 };

struct Object : ListNode<> {};

struct RefObject : RefCounted<RefObject, NoopAllocation>, ListNode<> {};

} // namespace

TEST_GROUP(list) {};

TEST(list, empty_list) {
    List<Object, NoOwnership> list;

    CHECK(list.front() == NULL);
    CHECK(list.back() == NULL);

    LONGS_EQUAL(0, list.size());
    CHECK(list.is_empty());
}

TEST(list, push_front) {
    { // one
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_front(objects[0]);

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[0], list.back());

        LONGS_EQUAL(1, list.size());
        CHECK(!list.is_empty());
    }
    { // many
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(i, list.size());

            list.push_front(objects[i]);
        }

        POINTERS_EQUAL(&objects[NumObjects - 1], list.front());
        POINTERS_EQUAL(&objects[0], list.back());

        LONGS_EQUAL(NumObjects, list.size());
        CHECK(!list.is_empty());
    }
}

TEST(list, push_back) {
    { // one
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[0]);

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[0], list.back());

        LONGS_EQUAL(1, list.size());
        CHECK(!list.is_empty());
    }
    { // many
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(i, list.size());

            list.push_back(objects[i]);
        }

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[NumObjects - 1], list.back());

        LONGS_EQUAL(NumObjects, list.size());
        CHECK(!list.is_empty());
    }
}

TEST(list, pop_front) {
    { // with push_back
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(i, list.size());

            list.push_back(objects[i]);
        }

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(NumObjects - i, list.size());

            list.pop_front();

            if (i != NumObjects - 1) {
                POINTERS_EQUAL(&objects[i + 1], list.front());
                POINTERS_EQUAL(&objects[NumObjects - 1], list.back());
            }
        }

        CHECK(list.front() == NULL);
        CHECK(list.back() == NULL);
        LONGS_EQUAL(0, list.size());
    }
    { // with push_front
        Object objects[NumObjects];
        List<Object, NoOwnership> list;
        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(i, list.size());
            list.push_front(objects[i]);
        }

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(NumObjects - i, list.size());
            list.pop_front();

            if (i != NumObjects - 1) {
                POINTERS_EQUAL(&objects[NumObjects - i - 2], list.front());
                POINTERS_EQUAL(&objects[0], list.back());
            }
        }

        CHECK(list.front() == NULL);
        CHECK(list.back() == NULL);
        LONGS_EQUAL(0, list.size());
    }
}

TEST(list, pop_back) {
    { // with push_back
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(i, list.size());

            list.push_back(objects[i]);
        }

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(NumObjects - i, list.size());

            list.pop_back();

            if (i != NumObjects - 1) {
                POINTERS_EQUAL(&objects[0], list.front());
                POINTERS_EQUAL(&objects[NumObjects - i - 2], list.back());
            }
        }
        CHECK(list.front() == NULL);
        CHECK(list.back() == NULL);
        LONGS_EQUAL(0, list.size());
    }
    { // with push_front
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(i, list.size());
            list.push_front(objects[i]);
        }

        for (size_t i = 0; i < NumObjects; ++i) {
            LONGS_EQUAL(NumObjects - i, list.size());
            list.pop_back();

            if (i != NumObjects - 1) {
                POINTERS_EQUAL(&objects[i + 1], list.back());
                POINTERS_EQUAL(&objects[NumObjects - 1], list.front());
            }
        }

        CHECK(list.front() == NULL);
        CHECK(list.back() == NULL);
        LONGS_EQUAL(0, list.size());
    }
}

TEST(list, insert_before) {
    { // front
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[1]);
        list.push_back(objects[2]);

        list.insert_before(objects[0], objects[1]);

        LONGS_EQUAL(3, list.size());
        CHECK(!list.is_empty());

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[2], list.back());

        int i = 0;
        for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
            POINTERS_EQUAL(&objects[i++], obj);
        }
        LONGS_EQUAL(i, list.size());
    }
    { // middle
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[0]);
        list.push_back(objects[1]);
        list.push_back(objects[3]);
        list.push_back(objects[4]);

        list.insert_before(objects[2], objects[3]);

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[4], list.back());

        LONGS_EQUAL(5, list.size());
        CHECK(!list.is_empty());

        int i = 0;
        for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
            POINTERS_EQUAL(&objects[i++], obj);
        }
        LONGS_EQUAL(i, list.size());
    }
    { // back
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[0]);
        list.push_back(objects[2]);

        list.insert_before(objects[1], objects[2]);

        LONGS_EQUAL(3, list.size());
        CHECK(!list.is_empty());

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[2], list.back());

        int i = 0;
        for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
            POINTERS_EQUAL(&objects[i++], obj);
        }
        LONGS_EQUAL(i, list.size());
    }
}

TEST(list, insert_after) {
    { // front
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[0]);
        list.push_back(objects[2]);

        list.insert_after(objects[1], objects[0]);

        LONGS_EQUAL(3, list.size());
        CHECK(!list.is_empty());

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[2], list.back());

        int i = 0;
        for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
            POINTERS_EQUAL(&objects[i++], obj);
        }
        LONGS_EQUAL(i, list.size());
    }
    { // middle
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[0]);
        list.push_back(objects[1]);
        list.push_back(objects[3]);
        list.push_back(objects[4]);

        list.insert_after(objects[2], objects[1]);

        LONGS_EQUAL(5, list.size());
        CHECK(!list.is_empty());

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[4], list.back());

        int i = 0;
        for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
            POINTERS_EQUAL(&objects[i++], obj);
        }
        LONGS_EQUAL(i, list.size());
    }
    { // back
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[0]);
        list.push_back(objects[1]);

        list.insert_after(objects[2], objects[1]);

        LONGS_EQUAL(3, list.size());
        CHECK(!list.is_empty());

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[2], list.back());

        int i = 0;
        for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
            POINTERS_EQUAL(&objects[i++], obj);
        }
        LONGS_EQUAL(i, list.size());
    }
}

TEST(list, remove) {
    { // front
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

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
        CHECK(list.is_empty());
    }
    { // middle
        Object objects[NumObjects];
        List<Object, NoOwnership> list;

        list.push_back(objects[0]);
        list.push_back(objects[1]);
        list.push_back(objects[2]);

        LONGS_EQUAL(3, list.size());
        CHECK(!list.is_empty());

        list.remove(objects[1]);

        POINTERS_EQUAL(&objects[0], list.front());
        POINTERS_EQUAL(&objects[2], list.back());
        POINTERS_EQUAL(list.back(), list.nextof(*list.front()));

        LONGS_EQUAL(2, list.size());
        CHECK(!list.is_empty());
    }
}

TEST(list, contains) {
    Object objects[NumObjects];
    List<Object, NoOwnership> list;

    CHECK(!list.contains(objects[0]));

    list.push_back(objects[0]);
    CHECK(list.contains(objects[0]));

    list.remove(objects[0]);
    CHECK(!list.contains(objects[0]));
}

TEST(list, iteration) {
    Object objects[NumObjects];
    List<Object, NoOwnership> list;

    for (size_t i = 0; i < NumObjects; ++i) {
        list.push_back(objects[i]);
    }

    {
        int i = 0;
        for (Object* obj = list.front(); obj != NULL; obj = list.nextof(*obj)) {
            POINTERS_EQUAL(&objects[i++], obj);
        }
    }

    {
        int i = NumObjects - 1;
        for (Object* obj = list.back(); obj != NULL; obj = list.prevof(*obj)) {
            POINTERS_EQUAL(&objects[i--], obj);
        }
    }
}

TEST(list, ownership_operations) {
    { // push_front
        RefObject obj;
        List<RefObject, RefCountedOwnership> list;

        LONGS_EQUAL(0, obj.getref());
        list.push_front(obj);
        LONGS_EQUAL(1, obj.getref());
    }
    { // push_back
        RefObject obj;
        List<RefObject, RefCountedOwnership> list;

        LONGS_EQUAL(0, obj.getref());
        list.push_back(obj);
        LONGS_EQUAL(1, obj.getref());
    }
    { // pop_front
        RefObject obj1;
        RefObject obj2;

        List<RefObject, RefCountedOwnership> list;

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
    { // pop_back
        RefObject obj1;
        RefObject obj2;

        List<RefObject, RefCountedOwnership> list;

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
    { // insert_before
        RefObject obj1;
        RefObject obj2;

        List<RefObject, RefCountedOwnership> list;

        list.push_back(obj1);
        list.insert_before(obj2, obj1);

        LONGS_EQUAL(1, obj1.getref());
        LONGS_EQUAL(1, obj2.getref());
    }
    { // insert_after
        RefObject obj1;
        RefObject obj2;

        List<RefObject, RefCountedOwnership> list;

        list.push_back(obj1);
        list.insert_after(obj2, obj1);

        LONGS_EQUAL(1, obj1.getref());
        LONGS_EQUAL(1, obj2.getref());
    }
    { // remove
        RefObject obj;
        List<RefObject, RefCountedOwnership> list;

        list.push_back(obj);
        LONGS_EQUAL(1, obj.getref());

        list.remove(obj);
        LONGS_EQUAL(0, obj.getref());
    }
}

TEST(list, ownership_destructor) {
    RefObject obj;

    {
        List<RefObject, RefCountedOwnership> list;

        list.push_back(obj);

        LONGS_EQUAL(1, obj.getref());
    }

    LONGS_EQUAL(0, obj.getref());
}

TEST(list, shared_pointers) {
    RefObject obj;
    List<RefObject, RefCountedOwnership> list;

    list.push_back(obj);

    POINTERS_EQUAL(&obj, list.front().get());
    POINTERS_EQUAL(&obj, list.back().get());

    LONGS_EQUAL(2, list.front()->getref());
    LONGS_EQUAL(2, list.back()->getref());
}

} // namespace core
} // namespace roc
