/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/macro_helpers.h"
#include "roc_core/secure_random.h"
#include "roc_core/stddefs.h"

#include <CppUTest/TestHarness.h>
#include <CppUTest/UtestMacros.h>

namespace roc {
namespace core {

TEST_GROUP(secure_random) {};

TEST(secure_random, buf_len) {
    unsigned char buf[64] = { 0 };

    bool ok = secure_random(&buf[5], 50);
    CHECK_TRUE(ok);

    for (size_t i = 0; i < 5; i++) {
        BYTES_EQUAL(0, buf[i]);
    }

    for (size_t i = 55; i < ROC_ARRAY_SIZE(buf); i++) {
        BYTES_EQUAL(0, buf[i]);
    }
}

TEST(secure_random, some_trivial_corner_cases) {
    CHECK_TRUE(secure_random(NULL, 0));

    uint32_t res32;
    CHECK_TRUE(secure_random_range_32(12345, 12345, res32));
    UNSIGNED_LONGS_EQUAL(12345, res32);

    uint64_t res64;
    CHECK_TRUE(secure_random_range_64(444555666, 444555666, res64));
    UNSIGNED_LONGS_EQUAL(444555666, res64);

    CHECK_TRUE(secure_random_range_32(0, UINT32_MAX, res32));
    CHECK_TRUE(secure_random_range_64(0, UINT64_MAX, res64));
}

TEST(secure_random, sec32) {
    uint32_t res32 = 0;

    for (uint32_t i = 0, j = 500; i < 250 && j > 250; i += 15, j -= 11) {
        CHECK_TRUE(secure_random_range_32(i, j, res32));
        CHECK(i <= res32 && res32 <= j);
    }
}

TEST(secure_random, sec64) {
    uint64_t res64 = 0;

    for (uint64_t i = 0, j = 500; i < 250 && j > 250; i += 15, j -= 11) {
        CHECK_TRUE(secure_random_range_64(i, j, res64));
        CHECK(i <= res64 && res64 <= j);
    }
}

} // namespace core
} // namespace roc
