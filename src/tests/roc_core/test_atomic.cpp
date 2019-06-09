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
    Atomic a1;
    CHECK(a1 == 0);

    Atomic a2(1);
    CHECK(a2 == 1);
}

TEST(atomic, store_load) {
    Atomic a;

    a = 1;
    CHECK(a == 1);
}

TEST(atomic, inc_dec) {
    Atomic a;

    CHECK(++a == 1);
    CHECK(a == 1);

    CHECK(--a == 0);
    CHECK(a == 0);
}

} // namespace core
} // namespace roc
