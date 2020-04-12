/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/atomic.h"
#include "roc_core/log.h"

namespace roc {
namespace core {

TEST_GROUP(atomic) {};

TEST(atomic, init_load) {
    Atomic a1;
    CHECK(a1 == 0);

    Atomic a2(123);
    CHECK(a2 == 123);

    Atomic a3(-123);
    CHECK(a3 == -123);
}

TEST(atomic, store_load) {
    Atomic a;

    a = 123;
    CHECK(a == 123);

    a = -123;
    CHECK(a == -123);
}

TEST(atomic, inc_dec) {
    Atomic a;

    CHECK(++a == 1);
    CHECK(a == 1);

    CHECK(++a == 2);
    CHECK(a == 2);

    CHECK(--a == 1);
    CHECK(a == 1);

    CHECK(--a == 0);
    CHECK(a == 0);
}

TEST(atomic, add_sub) {
    Atomic a;

    CHECK((a += 10) == 10);
    CHECK(a == 10);

    CHECK((a += 10) == 20);
    CHECK(a == 20);

    CHECK((a -= 30) == -10);
    CHECK(a == -10);

    CHECK((a -= 10) == -20);
    CHECK(a == -20);
}

TEST(atomic, boundaries) {
    const unsigned long max_ulong = (unsigned long)-1L;

    const long max_long = max_ulong / 2;
    const long min_long = -max_long - 1;

    Atomic a;

    a = min_long;
    CHECK(a == min_long);
    CHECK(--a == max_long);

    a = max_long;
    CHECK(a == max_long);
    CHECK(++a == min_long);
}

} // namespace core
} // namespace roc
