/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/format_time.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

namespace {

enum {
    // Timestamp length, including zero terminator.
    TsLen = sizeof("00:00:00.000")
};

} // namespace

TEST_GROUP(format_time) {};

TEST(format_time, buffer_size) {
    char buf[64];

    for (size_t i = 0; i < TsLen; i++) {
        CHECK(!format_time(buf, i));
    }

    for (size_t i = TsLen; i < sizeof(buf); i++) {
        CHECK(format_time(buf, i));
    }
}

TEST(format_time, zero_terminator) {
    char buf[64];
    memset(buf, 'x', sizeof(buf));

    CHECK(format_time(buf, sizeof(buf) - 10));

    for (size_t i = 0; i < TsLen - 1; i++) {
        CHECK(buf[i] != '\0' && buf[i] != 'x');
    }

    CHECK(buf[TsLen - 1] == '\0');

    for (size_t i = TsLen; i < sizeof(buf); i++) {
        CHECK(buf[i] == 'x');
    }
}

} // namespace core
} // namespace roc
