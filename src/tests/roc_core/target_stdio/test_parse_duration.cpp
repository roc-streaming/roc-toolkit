/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/parse_duration.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

TEST_GROUP(parse_duration) {};

TEST(parse_duration, error) {
    nanoseconds_t result = 0;

    CHECK(!parse_duration(NULL, result));
    CHECK(!parse_duration("", result));
    CHECK(!parse_duration("1", result));
    CHECK(!parse_duration("s", result));
    CHECK(!parse_duration("1 s", result));
    CHECK(!parse_duration(" 1s", result));
    CHECK(!parse_duration("1s ", result));
    CHECK(!parse_duration("!s", result));
    CHECK(!parse_duration("s1", result));
    CHECK(!parse_duration("1x", result));
}

TEST(parse_duration, parse) {
    nanoseconds_t result = 0;

    CHECK(parse_duration("123ns", result));
    CHECK(result == 123 * Nanosecond);

    CHECK(parse_duration("123us", result));
    CHECK(result == 123 * Microsecond);

    CHECK(parse_duration("123ms", result));
    CHECK(result == 123 * Millisecond);

    CHECK(parse_duration("123s", result));
    CHECK(result == 123 * Second);

    CHECK(parse_duration("123m", result));
    CHECK(result == 123 * Minute);

    CHECK(parse_duration("123h", result));
    CHECK(result == 123 * Hour);
}

} // namespace core
} // namespace roc
