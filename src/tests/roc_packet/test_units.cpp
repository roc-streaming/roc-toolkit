/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/units.h"

namespace roc {
namespace packet {

TEST_GROUP(units) {};

TEST(units, seqnum_diff) {
    const seqnum_t v = 65535;

    LONGS_EQUAL(0, seqnum_diff(v, v));

    LONGS_EQUAL(+1, seqnum_diff(seqnum_t(v + 1), v));
    LONGS_EQUAL(-1, seqnum_diff(seqnum_t(v - 1), v));

    CHECK(seqnum_lt(v / 2, v));
    CHECK(seqnum_diff(v / 2, v) < 0);

    CHECK(!seqnum_lt(v / 2 - 1, v));
    CHECK(seqnum_diff(v / 2 - 1, v) > 0);
}

TEST(units, seqnum_lt) {
    const seqnum_t v = 65535;

    CHECK(seqnum_lt(seqnum_t(v - 1), v));
    CHECK(seqnum_lt(seqnum_t(v - 5), v));

    CHECK(!seqnum_lt(seqnum_t(v + 1), v));
    CHECK(!seqnum_lt(seqnum_t(v + 5), v));

    CHECK(seqnum_lt(v / 2, v));
    CHECK(!seqnum_lt(v / 2 - 1, v));
}

TEST(units, seqnum_le) {
    const seqnum_t v = 65535;

    CHECK(!seqnum_lt(v, v));
    CHECK(seqnum_le(v, v));

    CHECK(seqnum_le(seqnum_t(v - 1), v));
    CHECK(seqnum_le(seqnum_t(v - 5), v));

    CHECK(!seqnum_le(seqnum_t(v + 1), v));
    CHECK(!seqnum_le(seqnum_t(v + 5), v));

    CHECK(seqnum_le(v / 2, v));
    CHECK(!seqnum_le(v / 2 - 1, v));
}

TEST(units, timestamp_diff) {
    const timestamp_t v = 4294967295u;

    LONGS_EQUAL(0, timestamp_diff(v, v));

    LONGS_EQUAL(+1, timestamp_diff(timestamp_t(v + 1), v));
    LONGS_EQUAL(-1, timestamp_diff(timestamp_t(v - 1), v));

    CHECK(timestamp_lt(v / 2, v));
    CHECK(timestamp_diff(v / 2, v) < 0);

    CHECK(!timestamp_lt(v / 2 - 1, v));
    CHECK(timestamp_diff(v / 2 - 1, v) > 0);
}

TEST(units, timestamp_lt) {
    const timestamp_t v = 4294967295u;

    CHECK(timestamp_lt(timestamp_t(v - 1), v));
    CHECK(timestamp_lt(timestamp_t(v - 5), v));

    CHECK(!timestamp_lt(timestamp_t(v + 1), v));
    CHECK(!timestamp_lt(timestamp_t(v + 5), v));

    CHECK(timestamp_lt(v / 2, v));
    CHECK(!timestamp_lt(v / 2 - 1, v));
}

TEST(units, timestamp_le) {
    const timestamp_t v = 4294967295u;

    CHECK(!timestamp_lt(v, v));
    CHECK(timestamp_le(v, v));

    CHECK(timestamp_le(timestamp_t(v - 1), v));
    CHECK(timestamp_le(timestamp_t(v - 5), v));

    CHECK(!timestamp_le(timestamp_t(v + 1), v));
    CHECK(!timestamp_le(timestamp_t(v + 5), v));

    CHECK(timestamp_le(v / 2, v));
    CHECK(!timestamp_le(v / 2 - 1, v));
}

TEST(units, num_channels) {
    enum { Ch1 = 0x1, Ch2 = 0x2, Ch3 = 0x4 };

    LONGS_EQUAL(0, num_channels(0));

    LONGS_EQUAL(1, num_channels(Ch1));
    LONGS_EQUAL(1, num_channels(Ch2));
    LONGS_EQUAL(1, num_channels(Ch3));

    LONGS_EQUAL(2, num_channels(Ch1 | Ch2));
    LONGS_EQUAL(2, num_channels(Ch2 | Ch3));
    LONGS_EQUAL(2, num_channels(Ch1 | Ch3));

    LONGS_EQUAL(3, num_channels(Ch1 | Ch2 | Ch3));
}

TEST(units, blknum_diff) {
    const blknum_t v = 65535;

    LONGS_EQUAL(0, blknum_diff(v, v));

    LONGS_EQUAL(+1, blknum_diff(blknum_t(v + 1), v));
    LONGS_EQUAL(-1, blknum_diff(blknum_t(v - 1), v));

    CHECK(blknum_lt(v / 2, v));
    CHECK(blknum_diff(v / 2, v) < 0);

    CHECK(!blknum_lt(v / 2 - 1, v));
    CHECK(blknum_diff(v / 2 - 1, v) > 0);
}

TEST(units, blknum_lt) {
    const blknum_t v = 65535;

    CHECK(blknum_lt(blknum_t(v - 1), v));
    CHECK(blknum_lt(blknum_t(v - 5), v));

    CHECK(!blknum_lt(blknum_t(v + 1), v));
    CHECK(!blknum_lt(blknum_t(v + 5), v));

    CHECK(blknum_lt(v / 2, v));
    CHECK(!blknum_lt(v / 2 - 1, v));
}

TEST(units, blknum_le) {
    const blknum_t v = 65535;

    CHECK(!blknum_lt(v, v));
    CHECK(blknum_le(v, v));

    CHECK(blknum_le(blknum_t(v - 1), v));
    CHECK(blknum_le(blknum_t(v - 5), v));

    CHECK(!blknum_le(blknum_t(v + 1), v));
    CHECK(!blknum_le(blknum_t(v + 5), v));

    CHECK(blknum_le(v / 2, v));
    CHECK(!blknum_le(v / 2 - 1, v));
}

} // namespace packet
} // namespace roc
