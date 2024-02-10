/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/seqlock.h"

namespace roc {
namespace core {

TEST_GROUP(seqlock) {};

TEST(seqlock, load_store) {
    Seqlock<int> sl(345);
    LONGS_EQUAL(345, sl.wait_load());

    sl.exclusive_store(123);
    LONGS_EQUAL(123, sl.wait_load());

    int v1 = 0;
    CHECK(sl.try_load(v1));
    LONGS_EQUAL(123, v1);

    CHECK(sl.try_store(456));
    LONGS_EQUAL(456, sl.wait_load());

    int v2 = 0;
    CHECK(sl.try_load(v2));
    LONGS_EQUAL(456, v2);
}

TEST(seqlock, version) {
    Seqlock<int> sl(0);
    seqlock_version_t v0 = sl.version();

    seqlock_version_t v1 = 0;
    CHECK(sl.try_store_v(1, v1));
    CHECK(v1 == sl.version());
    CHECK(v1 != v0);

    seqlock_version_t v2 = 0;
    sl.exclusive_store_v(2, v2);
    CHECK(v2 == sl.version());
    CHECK(v2 != v1);
    CHECK(v2 != v0);

    seqlock_version_t v2r1 = 0;
    int val1 = 0;
    CHECK(sl.try_load_v(val1, v2r1));
    CHECK(v2r1 == v2);

    seqlock_version_t v2r2 = 0;
    int val2 = 0;
    sl.wait_load_v(val2, v2r2);
    CHECK(v2r2 == v2);
}

} // namespace core
} // namespace roc
