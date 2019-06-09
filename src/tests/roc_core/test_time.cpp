/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/time.h"

namespace roc {
namespace core {

TEST_GROUP(time) {};

TEST(time, timestamp) {
    const nanoseconds_t ts = timestamp();

    while (ts == timestamp()) {
        // wait one nanosecond
    }
}

TEST(time, sleep_until) {
    const nanoseconds_t ts = timestamp();

    sleep_until(ts + Millisecond);

    CHECK(timestamp() >= ts + Millisecond);
}

TEST(time, sleep_for) {
    const nanoseconds_t ts = timestamp();

    sleep_for(Millisecond);

    CHECK(timestamp() >= ts + Millisecond);
}

} // namespace core
} // namespace roc
