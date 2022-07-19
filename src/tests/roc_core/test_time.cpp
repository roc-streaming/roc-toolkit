/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/macro_helpers.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

namespace {

const clock_t clock_list[] = {
    core::ClockMonotonic,
    core::ClockUnix,
};

} // namespace

TEST_GROUP(time) {};

TEST(time, timestamp) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(clock_list); i++) {
        const clock_t clock = clock_list[i];

        const nanoseconds_t ts = timestamp(clock);

        while (ts == timestamp(clock)) {
            // wait one nanosecond
        }
    }
}

TEST(time, sleep_until) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(clock_list); i++) {
        const clock_t clock = clock_list[i];

        const nanoseconds_t ts = timestamp(clock);

        sleep_until(clock, ts + Millisecond);

        CHECK(timestamp(clock) >= ts + Millisecond);
    }
}

TEST(time, sleep_for) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(clock_list); i++) {
        const clock_t clock = clock_list[i];

        const nanoseconds_t ts = timestamp(clock);

        sleep_for(clock, Millisecond);

        CHECK(timestamp(clock) >= ts + Millisecond);
    }
}

} // namespace core
} // namespace roc
