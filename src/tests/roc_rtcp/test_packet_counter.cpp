/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_rtcp/packet_counter.h"

namespace roc {
namespace rtcp {

TEST_GROUP(packet_counter) {};

TEST(packet_counter, no_wrap) {
    PacketCounter pc;

    UNSIGNED_LONGS_EQUAL(10, pc.update(200, 210));
    UNSIGNED_LONGS_EQUAL(30, pc.update(200, 230));
    UNSIGNED_LONGS_EQUAL(30, pc.update(200, 220));
    UNSIGNED_LONGS_EQUAL(40, pc.update(200, 240));

    UNSIGNED_LONGS_EQUAL(10, pc.update(100, 110));
    UNSIGNED_LONGS_EQUAL(20, pc.update(100, 120));

    UNSIGNED_LONGS_EQUAL(10, pc.update(300, 310));
    UNSIGNED_LONGS_EQUAL(20, pc.update(300, 320));
}

TEST(packet_counter, wrap) {
    PacketCounter pc;

    UNSIGNED_LONGS_EQUAL(10, pc.update(0xFFFFFFFF - 30, 0xFFFFFFFF - 20));
    UNSIGNED_LONGS_EQUAL(20, pc.update(0xFFFFFFFF - 30, 0xFFFFFFFF - 10));
    UNSIGNED_LONGS_EQUAL(41, pc.update(0xFFFFFFFF - 30, 10));
    UNSIGNED_LONGS_EQUAL(61, pc.update(0xFFFFFFFF - 30, 30));
    UNSIGNED_LONGS_EQUAL(61, pc.update(0xFFFFFFFF - 30, 20));
    UNSIGNED_LONGS_EQUAL(71, pc.update(0xFFFFFFFF - 30, 40));

    UNSIGNED_LONGS_EQUAL(10, pc.update(10, 20));
}

} // namespace rtcp
} // namespace roc
