/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/atomic.h"

namespace roc {
namespace core {

TEST_GROUP(atomic) {};

TEST(atomic, init_load) {
    { // int
        Atomic<int> a1(0);
        CHECK(a1 == 0);

        Atomic<int> a2(123);
        CHECK(a2 == 123);

        Atomic<int> a3(-1);
        CHECK(a3 == -1);
    }
    { // ptr
        Atomic<int*> a1(NULL);
        CHECK(a1 == (int*)NULL);

        Atomic<int*> a2((int*)123);
        CHECK(a2 == (int*)123);

        Atomic<int*> a3((int*)-1);
        CHECK(a3 == (int*)-1);
    }
}

TEST(atomic, store_load) {
    { // int
        Atomic<int> a(0);

        a = 123;
        CHECK(a == 123);

        a = 456;
        CHECK(a == 456);
    }
    { // ptr
        Atomic<int*> a(NULL);

        a = (int*)123;
        CHECK(a == (int*)123);

        a = (int*)456;
        CHECK(a == (int*)456);
    }
}

TEST(atomic, inc_dec) {
    { // int
        Atomic<int> a(0);

        CHECK(++a == 1);
        CHECK(a == 1);

        CHECK(++a == 2);
        CHECK(a == 2);

        CHECK(--a == 1);
        CHECK(a == 1);

        CHECK(--a == 0);
        CHECK(a == 0);
    }
    { // ptr
        int arr[3];

        Atomic<int*> a(&arr[0]);

        CHECK(++a == &arr[1]);
        CHECK(a == &arr[1]);

        CHECK(++a == &arr[2]);
        CHECK(a == &arr[2]);

        CHECK(--a == &arr[1]);
        CHECK(a == &arr[1]);

        CHECK(--a == &arr[0]);
        CHECK(a == &arr[0]);
    }
}

TEST(atomic, add_sub) {
    { // int
        Atomic<int> a(0);

        CHECK((a += 10) == 10);
        CHECK(a == 10);

        CHECK((a += 10) == 20);
        CHECK(a == 20);

        CHECK((a -= 30) == -10);
        CHECK(a == -10);

        CHECK((a -= 10) == -20);
        CHECK(a == -20);
    }
    { // ptr
        int arr[50];

        Atomic<int*> a(&arr[20]);

        CHECK((a += 10) == &arr[30]);
        CHECK(a == &arr[30]);

        CHECK((a += 10) == &arr[40]);
        CHECK(a == &arr[40]);

        CHECK((a -= 30) == &arr[10]);
        CHECK(a == &arr[10]);

        CHECK((a -= 10) == &arr[0]);
        CHECK(a == &arr[0]);
    }
}

TEST(atomic, wrapping) {
    { // int
        const unsigned int max_uint = (unsigned int)-1L;

        const int max_int = max_uint / 2;
        const int min_int = -max_int - 1;

        Atomic<int> a(0);

        a = min_int;
        CHECK(a == min_int);
        CHECK(--a == max_int);

        a = max_int;
        CHECK(a == max_int);
        CHECK(++a == min_int);
    }
}

TEST(atomic, exchange) {
    { // ptr
        Atomic<int*> a((int*)123);

        CHECK(a.exchange((int*)456) == (int*)123);
        CHECK(a == (int*)456);
    }
}

TEST(atomic, compare_exchange) {
    { // ptr
        Atomic<int*> a((int*)123);

        CHECK(!a.compare_exchange((int*)456, (int*)789));
        CHECK(a == (int*)123);

        CHECK(a.compare_exchange((int*)123, (int*)789));
        CHECK(a == (int*)789);
    }
}

} // namespace core
} // namespace roc
