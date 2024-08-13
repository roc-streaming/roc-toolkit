/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/mpsc_queue.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

namespace {

struct Object : RefCounted<Object, NoopAllocation>, MpscQueueNode<> {};

} // namespace

TEST_GROUP(mpsc_queue) {};

TEST(mpsc_queue, empty) {
    MpscQueue<Object, NoOwnership> queue;

    POINTERS_EQUAL(NULL, queue.try_pop_front_exclusive());
    POINTERS_EQUAL(NULL, queue.pop_front_exclusive());
}

TEST(mpsc_queue, push_pop) {
    { // try_pop_front
        MpscQueue<Object, NoOwnership> queue;
        Object obj;

        for (int i = 0; i < 5; i++) {
            POINTERS_EQUAL(NULL, obj.mpsc_queue_data()->queue);

            queue.push_back(obj);

            POINTERS_EQUAL(&queue, obj.mpsc_queue_data()->queue);

            POINTERS_EQUAL(&obj, queue.try_pop_front_exclusive());
            POINTERS_EQUAL(NULL, queue.try_pop_front_exclusive());

            POINTERS_EQUAL(NULL, obj.mpsc_queue_data()->queue);
        }
    }
    { // pop_front
        MpscQueue<Object, NoOwnership> queue;
        Object obj;

        for (int i = 0; i < 5; i++) {
            POINTERS_EQUAL(NULL, obj.mpsc_queue_data()->queue);

            queue.push_back(obj);

            POINTERS_EQUAL(&queue, obj.mpsc_queue_data()->queue);

            POINTERS_EQUAL(&obj, queue.pop_front_exclusive());
            POINTERS_EQUAL(NULL, queue.pop_front_exclusive());

            POINTERS_EQUAL(NULL, obj.mpsc_queue_data()->queue);
        }
    }
}

TEST(mpsc_queue, push_pop_many) {
    enum { NumObjs = 10 };

    { // try_pop_front
        MpscQueue<Object, NoOwnership> queue;
        Object objs[NumObjs];

        for (int i = 0; i < 5; i++) {
            for (int n = 0; n < NumObjs; n++) {
                queue.push_back(objs[n]);
            }

            for (int n = 0; n < NumObjs; n++) {
                POINTERS_EQUAL(&objs[n], queue.try_pop_front_exclusive());
            }

            POINTERS_EQUAL(NULL, queue.try_pop_front_exclusive());
        }
    }

    { // pop_front
        MpscQueue<Object, NoOwnership> queue;
        Object objs[NumObjs];

        for (int i = 0; i < 5; i++) {
            for (int n = 0; n < NumObjs; n++) {
                queue.push_back(objs[n]);
            }

            for (int n = 0; n < NumObjs; n++) {
                POINTERS_EQUAL(&objs[n], queue.pop_front_exclusive());
            }

            POINTERS_EQUAL(NULL, queue.pop_front_exclusive());
        }
    }
}

TEST(mpsc_queue, ownership) {
    MpscQueue<Object, RefCountedOwnership> queue;

    Object obj1;
    Object obj2;

    UNSIGNED_LONGS_EQUAL(0, obj1.getref());
    UNSIGNED_LONGS_EQUAL(0, obj2.getref());

    queue.push_back(obj1);
    queue.push_back(obj2);

    UNSIGNED_LONGS_EQUAL(1, obj1.getref());
    UNSIGNED_LONGS_EQUAL(1, obj2.getref());

    {
        SharedPtr<Object> ptr1 = queue.pop_front_exclusive();
        SharedPtr<Object> ptr2 = queue.try_pop_front_exclusive();

        POINTERS_EQUAL(&obj1, ptr1.get());
        POINTERS_EQUAL(&obj2, ptr2.get());

        UNSIGNED_LONGS_EQUAL(1, obj1.getref());
        UNSIGNED_LONGS_EQUAL(1, obj2.getref());
    }

    UNSIGNED_LONGS_EQUAL(0, obj1.getref());
    UNSIGNED_LONGS_EQUAL(0, obj2.getref());
}

} // namespace core
} // namespace roc
