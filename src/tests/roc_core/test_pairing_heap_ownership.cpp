/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/pairing_heap.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

namespace {

struct NoAllocation {
    template <class T> void destroy(T&) {
    }
};

struct Object : RefCounted<Object, NoAllocation>, PairingHeapNode {};

typedef PairingHeap<Object, RefCountedOwnership> TestPairingHeap;

} // namespace

TEST_GROUP(pairing_heap_ownership) {};

TEST(pairing_heap_ownership, push) {
    Object obj;

    TestPairingHeap pairing_heap;

    LONGS_EQUAL(0, obj.getref());

    pairing_heap.push(obj);

    LONGS_EQUAL(1, obj.getref());
}

TEST(pairing_heap_ownership, push_as_child) {
    Object obj1;
    Object obj2;

    TestPairingHeap pairing_heap;

    pairing_heap.push(obj1);
    pairing_heap.push_as_child(obj2, obj1);

    LONGS_EQUAL(1, obj1.getref());
    LONGS_EQUAL(1, obj2.getref());
}

TEST(pairing_heap_ownership, push_as_parent) {
    Object obj1;
    Object obj2;

    TestPairingHeap pairing_heap;

    pairing_heap.push(obj1);
    pairing_heap.push_as_parent(obj2, obj1);

    LONGS_EQUAL(1, obj1.getref());
    LONGS_EQUAL(1, obj2.getref());
}

TEST(pairing_heap_ownership, merge) {
    Object obj1;
    Object obj2;
    Object obj3;

    TestPairingHeap pairing_heap;

    pairing_heap.push(obj1);
    pairing_heap.push_as_child(obj2, obj1);
    pairing_heap.push_as_child(obj3, obj1);
    pairing_heap.merge(obj2, obj3);

    LONGS_EQUAL(1, obj1.getref());
    LONGS_EQUAL(1, obj2.getref());
    LONGS_EQUAL(1, obj3.getref());
}

TEST(pairing_heap_ownership, remove) {
    Object obj;

    TestPairingHeap pairing_heap;

    pairing_heap.push(obj);

    LONGS_EQUAL(1, obj.getref());

    pairing_heap.remove(obj);

    LONGS_EQUAL(0, obj.getref());
}

TEST(pairing_heap_ownership, destructor) {
    Object obj;

    {
        TestPairingHeap pairing_heap;

        pairing_heap.push(obj);

        LONGS_EQUAL(1, obj.getref());
    }

    LONGS_EQUAL(0, obj.getref());
}

TEST(pairing_heap_ownership, pointers) {
    Object obj;

    TestPairingHeap pairing_heap;

    pairing_heap.push(obj);

    POINTERS_EQUAL(&obj, pairing_heap.top().get());

    LONGS_EQUAL(2, pairing_heap.top()->getref());
}

} // namespace core
} // namespace roc
