/*
 * Copyright (c) 2015 Roc Streaming authors
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

TEST(units, timestamp_diff) {
    const stream_timestamp_t v = 4294967295u;

    LONGS_EQUAL(0, stream_timestamp_diff(v, v));

    LONGS_EQUAL(+1, stream_timestamp_diff(stream_timestamp_t(v + 1), v));
    LONGS_EQUAL(-1, stream_timestamp_diff(stream_timestamp_t(v - 1), v));

    CHECK(stream_timestamp_lt(v / 2, v));
    CHECK(stream_timestamp_diff(v / 2, v) < 0);

    CHECK(!stream_timestamp_lt(v / 2 - 1, v));
    CHECK(stream_timestamp_diff(v / 2 - 1, v) > 0);
}

TEST(units, timestamp_lt) {
    const stream_timestamp_t v = 4294967295u;

    CHECK(stream_timestamp_lt(stream_timestamp_t(v - 1), v));
    CHECK(stream_timestamp_lt(stream_timestamp_t(v - 5), v));

    CHECK(!stream_timestamp_lt(stream_timestamp_t(v + 1), v));
    CHECK(!stream_timestamp_lt(stream_timestamp_t(v + 5), v));

    CHECK(stream_timestamp_lt(v / 2, v));
    CHECK(!stream_timestamp_lt(v / 2 - 1, v));
}

TEST(units, timestamp_le) {
    const stream_timestamp_t v = 4294967295u;

    CHECK(!stream_timestamp_lt(v, v));
    CHECK(stream_timestamp_le(v, v));

    CHECK(stream_timestamp_le(stream_timestamp_t(v - 1), v));
    CHECK(stream_timestamp_le(stream_timestamp_t(v - 5), v));

    CHECK(!stream_timestamp_le(stream_timestamp_t(v + 1), v));
    CHECK(!stream_timestamp_le(stream_timestamp_t(v + 5), v));

    CHECK(stream_timestamp_le(v / 2, v));
    CHECK(!stream_timestamp_le(v / 2 - 1, v));
}

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
