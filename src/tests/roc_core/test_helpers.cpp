/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/helpers.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace test {

TEST_GROUP(helpers){};

TEST(helpers, is_before) {
    const uint16_t v = 65535;

    CHECK(ROC_IS_BEFORE(int16_t, v - 1, v));
    CHECK(ROC_IS_BEFORE(int16_t, v - 5, v));

    CHECK(!ROC_IS_BEFORE(int16_t, v + 1, v));
    CHECK(!ROC_IS_BEFORE(int16_t, v + 5, v));

    CHECK(ROC_IS_BEFORE(int16_t, v / 2, v));
    CHECK(!ROC_IS_BEFORE(int16_t, v / 2 - 1, v));
}

TEST(helpers, is_before_eq) {
    const uint16_t v = 65535;

    CHECK(!ROC_IS_BEFORE(int16_t, v, v));
    CHECK(ROC_IS_BEFORE_EQ(int16_t, v, v));

    CHECK(ROC_IS_BEFORE_EQ(int16_t, v - 1, v));
    CHECK(ROC_IS_BEFORE_EQ(int16_t, v - 5, v));

    CHECK(!ROC_IS_BEFORE_EQ(int16_t, v + 1, v));
    CHECK(!ROC_IS_BEFORE_EQ(int16_t, v + 5, v));

    CHECK(ROC_IS_BEFORE_EQ(int16_t, v / 2, v));
    CHECK(!ROC_IS_BEFORE_EQ(int16_t, v / 2 - 1, v));
}

TEST(helpers, subtract) {
    const uint16_t v = 65535;

    LONGS_EQUAL(0, ROC_SUBTRACT(int16_t, v, v));

    LONGS_EQUAL(+1, ROC_SUBTRACT(int16_t, v + 1, v));
    LONGS_EQUAL(-1, ROC_SUBTRACT(int16_t, v - 1, v));

    CHECK(ROC_IS_BEFORE(int16_t, v / 2, v));
    CHECK(ROC_SUBTRACT(int16_t, v / 2, v) < 0);

    CHECK(!ROC_IS_BEFORE(int16_t, v / 2 - 1, v));
    CHECK(ROC_SUBTRACT(int16_t, v / 2 - 1, v) > 0);
}

} // namespace test
} // namespace roc
