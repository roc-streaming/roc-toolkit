/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// see also test_secure_random.cpp

#include <CppUTest/TestHarness.h>

#include "roc_core/fast_random.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

TEST_GROUP(fast_random) {};

TEST(fast_random, range_min_min) {
    uint64_t lo_hi = 0;
    uint64_t res = fast_random_range(lo_hi, lo_hi);

    LONGS_EQUAL(res, lo_hi);
}

TEST(fast_random, range_max_max) {
    uint64_t lo_hi = UINT64_MAX;
    uint64_t res = fast_random_range(lo_hi, lo_hi);

    LONGS_EQUAL(res, lo_hi);
}

TEST(fast_random, range_loop) {
    for (int i = 0; i < 10000; i++) {
        uint64_t res = fast_random_range(300, 400);

        CHECK(res >= 300);
        CHECK(res <= 400);
    }
}

TEST(fast_random, float_loop) {
    for (int i = 0; i < 10000; i++) {
        float res = fast_random_float();

        CHECK(res >= 0);
        CHECK(res <= 1);
    }
}

} // namespace core
} // namespace roc
