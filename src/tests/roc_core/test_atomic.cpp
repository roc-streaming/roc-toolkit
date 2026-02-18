/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/atomic_bool.h"
#include "roc_core/atomic_int.h"
#include "roc_core/atomic_ptr.h"
#include "roc_core/atomic_size.h"

namespace roc {
namespace core {

TEST_GROUP(atomic) {};

TEST(atomic, init_load) {
    { // AtomicInt
        AtomicInt<int32_t> a1;
        CHECK(a1 == 0);

        AtomicInt<int32_t> a2(123);
        CHECK(a2 == 123);
    }
    { // AtomicSize
        AtomicSize a1;
        CHECK(a1 == 0);

        AtomicSize a2(123);
        CHECK(a2 == 123);
    }
    { // AtomicBool
        AtomicBool a1;
        CHECK(a1 == false);

        AtomicBool a2(true);
        CHECK(a2 == true);
    }
    { // AtomicPtr
        AtomicPtr<char> a1;
        CHECK(a1 == nullptr);

        char s[] = "test";
        AtomicPtr<char> a2(s);
        CHECK(a2 == s);
    }
}

TEST(atomic, store_load) {
    { // AtomicInt
        AtomicInt<int32_t> a;

        a = 123;
        CHECK(a == 123);

        a = 456;
        CHECK(a == 456);
    }
    { // AtomicSize
        AtomicSize a;

        a = 123;
        CHECK(a == 123);

        a = 456;
        CHECK(a == 456);
    }
    { // AtomicBool
        AtomicBool a;

        a = true;
        CHECK(a == true);

        a = false;
        CHECK(a == false);
    }
    { // AtomicPtr
        AtomicPtr<char> a;

        char s1[] = "test";
        a = s1;
        CHECK(a == s1);

        char s2[] = "test";
        a = s2;
        CHECK(a == s2);
    }
}

TEST(atomic, inc_dec) {
    { // AtomicInt
        AtomicInt<int32_t> a;

        CHECK(++a == 1);
        CHECK(a == 1);

        CHECK(++a == 2);
        CHECK(a == 2);

        CHECK(--a == 1);
        CHECK(a == 1);

        CHECK(--a == 0);
        CHECK(a == 0);
    }
    { // AtomicSize
        AtomicSize a;

        CHECK(++a == 1);
        CHECK(a == 1);

        CHECK(++a == 2);
        CHECK(a == 2);

        CHECK(--a == 1);
        CHECK(a == 1);

        CHECK(--a == 0);
        CHECK(a == 0);
    }
    { // AtomicPtr
        char arr[50];

        AtomicPtr<char> a(&arr[0]);

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
    { // AtomicInt
        AtomicInt<int32_t> a;

        CHECK((a += 10) == 10);
        CHECK(a == 10);

        CHECK((a += 10) == 20);
        CHECK(a == 20);

        CHECK((a -= 30) == -10);
        CHECK(a == -10);

        CHECK((a -= 10) == -20);
        CHECK(a == -20);
    }
    { // AtomicSize
        AtomicSize a;

        CHECK((a += 10) == 10);
        CHECK(a == 10);

        CHECK((a += 10) == 20);
        CHECK(a == 20);

        CHECK((a -= 5) == 15);
        CHECK(a == 15);

        CHECK((a -= 10) == 5);
        CHECK(a == 5);
    }
    { // AtomicPtr
        char arr[50];

        AtomicPtr<char> a(&arr[20]);

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
    { // AtomicInt
        const uint32_t max_uint32_t = (uint32_t)-1L;

        AtomicInt<uint32_t> a;

        a = 0;
        CHECK(a == 0);
        CHECK(--a == max_uint32_t);

        a = max_uint32_t;
        CHECK(a == max_uint32_t);
        CHECK(++a == 0);
    }
    { // AtomicSize
        const size_t max_size_t = (size_t)-1L;

        AtomicSize a;

        a = 0;
        CHECK(a == 0);
        CHECK(--a == max_size_t);

        a = max_size_t;
        CHECK(a == max_size_t);
        CHECK(++a == 0);
    }
}

TEST(atomic, bit_ops) {
    { // AtomicInt (operators)
        AtomicInt<int32_t> a(0x000);

        CHECK((a |= 0x011) == 0x011);
        CHECK(a == 0x011);

        CHECK((a &= 0x110) == 0x010);
        CHECK(a == 0x010);

        CHECK((a ^= 0x100) == 0x110);
        CHECK(a == 0x110);
    }
    { // AtomicInt (methods)
        AtomicInt<int32_t> a(0x000);

        CHECK(a.fetch_or(0x011) == 0x000);
        CHECK(a == 0x011);

        CHECK(a.fetch_and(0x110) == 0x011);
        CHECK(a == 0x010);

        CHECK(a.fetch_xor(0x100) == 0x010);
        CHECK(a == 0x110);
    }
}

TEST(atomic, exchange) {
    { // AtomicInt
        AtomicInt<int32_t> a(123);

        CHECK(a.exchange(456) == 123);
        CHECK(a == 456);
    }
    { // AtomicSize
        AtomicSize a(123);

        CHECK(a.exchange(456) == 123);
        CHECK(a == 456);
    }
    { // AtomicBool
        AtomicBool a(true);

        CHECK(a.exchange(false) == true);
        CHECK(a == false);

        CHECK(a.exchange(true) == false);
        CHECK(a == true);
    }
    { // AtomicPtr
        char s1[] = "test";
        char s2[] = "test";

        AtomicPtr<char> a(s1);

        CHECK(a.exchange(s2) == s1);
        CHECK(a == s2);
    }
}

TEST(atomic, compare_exchange) {
    { // AtomicInt
        AtomicInt<int32_t> a(123);

        CHECK(!a.compare_exchange(456, 789));
        CHECK(a == 123);

        CHECK(a.compare_exchange(123, 789));
        CHECK(a == 789);
    }
    { // AtomicSize
        AtomicSize a(123);

        CHECK(!a.compare_exchange(456, 789));
        CHECK(a == 123);

        CHECK(a.compare_exchange(123, 789));
        CHECK(a == 789);
    }
    { // AtomicBool
        AtomicBool a(true);

        CHECK(!a.compare_exchange(false, true));
        CHECK(a == true);

        CHECK(a.compare_exchange(true, false));
        CHECK(a == false);
    }
    { // AtomicPtr
        char s1[] = "test";
        char s2[] = "test";
        char s3[] = "test";

        AtomicPtr<char> a(s1);

        CHECK(!a.compare_exchange(s2, s3));
        CHECK(a == s1);

        CHECK(a.compare_exchange(s1, s3));
        CHECK(a == s3);
    }
}

} // namespace core
} // namespace roc
