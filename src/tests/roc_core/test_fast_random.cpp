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

TEST(fast_random, min_min) {
    uint32_t lo_hi = 0;
    uint32_t res = fast_random_range(lo_hi, lo_hi);

    LONGS_EQUAL(res, lo_hi);
}

TEST(fast_random, max_max) {
    uint32_t lo_hi = UINT32_MAX;
    uint32_t res = fast_random_range(lo_hi, lo_hi);

    LONGS_EQUAL(res, lo_hi);
}

TEST(fast_random, limits) {
    uint32_t res = fast_random_range(1, 100);

    CHECK(1 <= res && res <= 100);
}

} // namespace core
} // namespace roc
