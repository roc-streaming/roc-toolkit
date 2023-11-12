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
}

TEST(parse_units, parse_duration) {
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
}

TEST(parse_units, parse_size) {
    size_t result = 0;

    const size_t kibibyte = 1024;
    const size_t mebibyte = 1024 * kibibyte;
    const size_t gibibyte = 1024 * mebibyte;

    CHECK(parse_size("123", result));
    CHECK(result == 123);

    CHECK(parse_size("123K", result));
    CHECK(result == 123 * kibibyte);

    CHECK(parse_size("123M", result));
    CHECK(result == 123 * mebibyte);

    CHECK(parse_size("1G", result));
    CHECK(result == 1 * gibibyte);
}

TEST(parse_units, parse_size_overflows_due_to_sizeof_size_t) {
    if (SIZE_MAX < ULLONG_MAX) {
        char s[32];
        snprintf(s, 32, "%llu", ULLONG_MAX - 1);
        size_t result = 0;
        CHECK_FALSE(parse_size(s, result));
    }
}

TEST(parse_units, parse_size_overflows_due_to_multiplier) {
    char s[32];
    snprintf(s, 32, "%zuK", SIZE_MAX - 1);
    size_t result = 0;
    CHECK_FALSE(parse_size(s, result));
}

} // namespace core
} // namespace roc
