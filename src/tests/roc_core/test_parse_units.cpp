/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/parse_units.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

TEST_GROUP(parse_units) {};

TEST(parse_units, parse_duration_error) {
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
    CHECK(!parse_duration("1.2.3s", result));
    CHECK(!parse_duration(".1s", result));
}

TEST(parse_units, parse_duration_suffix) {
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

TEST(parse_units, parse_duration_sign) {
    nanoseconds_t result = 0;

    CHECK(parse_duration("123ms", result));
    CHECK(result == 123 * Millisecond);

    CHECK(parse_duration("+123ms", result));
    CHECK(result == 123 * Millisecond);

    CHECK(parse_duration("-123ms", result));
    CHECK(result == -123 * Millisecond);
}

TEST(parse_units, parse_duration_float_le_one) {
    nanoseconds_t result = 0;

    CHECK(parse_duration("0.ns", result));
    CHECK(result == 0);
    CHECK(parse_duration("0.0ns", result));
    CHECK(result == 0);

    CHECK(parse_duration("0.1ns", result));
    CHECK(result == 0);

    CHECK(parse_duration("0.0001us", result));
    CHECK(result == 0);
    CHECK(parse_duration("0.1us", result));
    CHECK(result == 100);

    CHECK(parse_duration("0.1ms", result));
    CHECK(result == 100000);

    CHECK(parse_duration("0.1s", result));
    CHECK(result == 100000000);
}

TEST(parse_units, parse_duration_float_gt_one) {
    nanoseconds_t result = 0;

    CHECK(parse_duration("1.ns", result));
    CHECK(result == 1);
    CHECK(parse_duration("1.1ns", result));
    CHECK(result == 1);
    CHECK(parse_duration("1.5ns", result));
    CHECK(result == 2);

    CHECK(parse_duration("1.1us", result));
    CHECK(result == 1100);

    CHECK(parse_duration("1.1ms", result));
    CHECK(result == 1100000);

    CHECK(parse_duration("1.1s", result));
    CHECK(result == 1100000000);
}

TEST(parse_units, parse_size_error) {
    size_t result = 0;

    CHECK(!parse_size(NULL, result));
    CHECK(!parse_size("", result));
    CHECK(!parse_size("K", result));
    CHECK(!parse_size("1 K", result));
    CHECK(!parse_size(" 1K", result));
    CHECK(!parse_size("1K ", result));
    CHECK(!parse_size("!K", result));
    CHECK(!parse_size("K1", result));
    CHECK(!parse_size("1x", result));
    CHECK(!parse_size("1.2.3K", result));
    CHECK(!parse_size(".1", result));
    CHECK(!parse_size(".1K", result));
}

TEST(parse_units, parse_size_suffix) {
    size_t result = 0;

    const size_t kibibyte = 1024;
    const size_t mebibyte = 1024 * kibibyte;
    const size_t gibibyte = 1024 * mebibyte;

    CHECK(parse_size("0", result));
    CHECK(result == 0);

    CHECK(parse_size("123", result));
    CHECK(result == 123);

    CHECK(parse_size("123K", result));
    CHECK(result == 123 * kibibyte);

    CHECK(parse_size("123M", result));
    CHECK(result == 123 * mebibyte);

    CHECK(parse_size("1G", result));
    CHECK(result == 1 * gibibyte);
}

TEST(parse_units, parse_size_float_le_one) {
    size_t result = 0;

    CHECK(parse_size("0.", result));
    CHECK(result == 0);
    CHECK(parse_size("0.0", result));
    CHECK(result == 0);

    CHECK(parse_size("0.1", result));
    CHECK(result == 0);

    CHECK(parse_size("0.0001K", result));
    CHECK(result == 0);
    CHECK(parse_size("0.1K", result));
    CHECK(result == 102);

    CHECK(parse_size("0.1M", result));
    CHECK(result == 104858);

    CHECK(parse_size("0.1G", result));
    CHECK(result == 107374182);
}

TEST(parse_units, parse_size_float_gt_one) {
    size_t result = 0;

    CHECK(parse_size("1.", result));
    CHECK(result == 1);
    CHECK(parse_size("1.1", result));
    CHECK(result == 1);
    CHECK(parse_size("1.5", result));
    CHECK(result == 2);

    CHECK(parse_size("1.1K", result));
    CHECK(result == 1126);

    CHECK(parse_size("1.1M", result));
    CHECK(result == 1153434);

    CHECK(parse_size("1.1G", result));
    CHECK(result == 1181116006);
}

TEST(parse_units, parse_size_overflow) {
    char s[32];
    snprintf(s, 32, "%lluK", (unsigned long long)SIZE_MAX);
    size_t result = 0;
    CHECK_FALSE(parse_size(s, result));
}

} // namespace core
} // namespace roc
