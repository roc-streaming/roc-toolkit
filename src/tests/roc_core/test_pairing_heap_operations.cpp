/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/pairing_heap.h"

namespace roc {
namespace core {

namespace {

enum { NumObjects = 5 };

struct Object : PairingHeapNode {};

} // namespace

TEST_GROUP(pairing_heap_operations) {
    Object objects[NumObjects];

    PairingHeap<Object, NoOwnership> pairing_heap;
};

TEST(pairing_heap_operations, empty) {
    CHECK(pairing_heap.top() == NULL);

    LONGS_EQUAL(0, pairing_heap.size());
}

TEST(pairing_heap_operations, push_one) {
    pairing_heap.push(objects[0]);

    POINTERS_EQUAL(&objects[0], pairing_heap.top());

    LONGS_EQUAL(1, pairing_heap.size());
}

TEST(pairing_heap_operations, push_as_child_many) {
    LONGS_EQUAL(0, pairing_heap.size());

    pairing_heap.push(objects[0]);

    for (size_t i = 1; i < NumObjects; ++i) {
        LONGS_EQUAL(i, pairing_heap.size());

        pairing_heap.push_as_child(objects[i], objects[i - 1]);
    }

    POINTERS_EQUAL(&objects[0], pairing_heap.top());

    LONGS_EQUAL(NumObjects, pairing_heap.size());
}

TEST(pairing_heap_operations, push_as_child_iterate) {
    pairing_heap.push(objects[0]);
    for (size_t i = 1; i < NumObjects; ++i) {
        pairing_heap.push_as_child(objects[i], objects[i - 1]);
    }

    int i = 0;
    for (Object* obj = pairing_heap.top(); obj != NULL;
         obj = pairing_heap.child_of(*obj)) {
        POINTERS_EQUAL(&objects[i++], obj);
    }
}

TEST(pairing_heap_operations, push_as_parent_many) {
    LONGS_EQUAL(0, pairing_heap.size());

    pairing_heap.push(objects[0]);

    for (size_t i = 1; i < NumObjects; ++i) {
        LONGS_EQUAL(i, pairing_heap.size());

        pairing_heap.push_as_parent(objects[i], objects[i - 1]);

        POINTERS_EQUAL(&objects[i], pairing_heap.top());
    }

    LONGS_EQUAL(NumObjects, pairing_heap.size());
}

TEST(pairing_heap_operations, push_as_parent_iterate) {
    pairing_heap.push(objects[0]);

    for (size_t i = 1; i < NumObjects; ++i) {
        pairing_heap.push_as_parent(objects[i], objects[i - 1]);
    }

    int i = NumObjects - 1;
    for (Object* obj = pairing_heap.top(); obj != NULL;
         obj = pairing_heap.child_of(*obj)) {
        POINTERS_EQUAL(&objects[i--], obj);
    }
}

TEST(pairing_heap_operations, remove_top) {
    pairing_heap.push(objects[0]);

    for (size_t i = 1; i < NumObjects; ++i) {
        pairing_heap.push_as_child(objects[i], objects[i - 1]);
    }

    for (size_t i = 0; i < NumObjects; ++i) {
        LONGS_EQUAL(NumObjects - i, pairing_heap.size());

        pairing_heap.remove(objects[i]);

        if (i != NumObjects - 1) {
            POINTERS_EQUAL(&objects[i + 1], pairing_heap.top());
        }
    }

    CHECK(pairing_heap.top() == NULL);

    LONGS_EQUAL(0, pairing_heap.size());
}

TEST(pairing_heap_operations, remove_middle) {
    pairing_heap.push(objects[0]);
    pairing_heap.push_as_child(objects[1], objects[0]);
    pairing_heap.push_as_child(objects[2], objects[1]);

    LONGS_EQUAL(3, pairing_heap.size());

    pairing_heap.remove(objects[1]);

    POINTERS_EQUAL(&objects[0], pairing_heap.top());

    pairing_heap.remove(objects[0]);

    POINTERS_EQUAL(&objects[2], pairing_heap.top());

    LONGS_EQUAL(1, pairing_heap.size());
}

TEST(pairing_heap_operations, siblings) {
    pairing_heap.push(objects[0]);
    pairing_heap.push_as_child(objects[1], objects[0]);
    pairing_heap.push_as_child(objects[2], objects[0]);

    LONGS_EQUAL(3, pairing_heap.size());

    POINTERS_EQUAL(&objects[2], pairing_heap.prev_sibling_of(objects[1]));
    POINTERS_EQUAL(&objects[1], pairing_heap.next_sibling_of(objects[2]));
}

TEST(pairing_heap_operations, contains) {
    CHECK(!pairing_heap.contains(objects[0]));

    pairing_heap.push(objects[0]);
    CHECK(pairing_heap.contains(objects[0]));

    pairing_heap.remove(objects[0]);
    CHECK(!pairing_heap.contains(objects[0]));
}

} // namespace core
} // namespace roc
