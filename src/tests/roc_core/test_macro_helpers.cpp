/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/macro_helpers.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

TEST_GROUP(macro_helpers) {};

TEST(macro_helpers, unsigned_min_max) {
    CHECK_EQUAL(0u, ROC_MIN_OF(uint8_t));
    CHECK_EQUAL(255u, ROC_MAX_OF(uint8_t));

    CHECK_EQUAL(0u, ROC_MIN_OF(uint16_t));
    CHECK_EQUAL(65535u, ROC_MAX_OF(uint16_t));

    CHECK_EQUAL(0ul, ROC_MIN_OF(uint32_t));
    CHECK_EQUAL(4294967295ul, ROC_MAX_OF(uint32_t));

    CHECK_EQUAL(0ull, ROC_MIN_OF(uint64_t));
    CHECK_EQUAL(18446744073709551615ull, ROC_MAX_OF(uint64_t));
}

TEST(macro_helpers, signed_min_max) {
    CHECK_EQUAL(-128, ROC_MIN_OF(int8_t));
    CHECK_EQUAL(127, ROC_MAX_OF(int8_t));

    CHECK_EQUAL(-32768, ROC_MIN_OF(int16_t));
    CHECK_EQUAL(32767, ROC_MAX_OF(int16_t));

    CHECK_EQUAL(-2147483647l - 1, ROC_MIN_OF(int32_t));
    CHECK_EQUAL(2147483647l, ROC_MAX_OF(int32_t));

    CHECK_EQUAL(-9223372036854775807ll - 1, ROC_MIN_OF(int64_t));
    CHECK_EQUAL(9223372036854775807ll, ROC_MAX_OF(int64_t));
}

} // namespace core
} // namespace roc
