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

    CHECK_EQUAL(10, pc.update(200, 210));
    CHECK_EQUAL(30, pc.update(200, 230));
    CHECK_EQUAL(30, pc.update(200, 220));
    CHECK_EQUAL(40, pc.update(200, 240));

    CHECK_EQUAL(10, pc.update(100, 110));
    CHECK_EQUAL(20, pc.update(100, 120));

    CHECK_EQUAL(10, pc.update(300, 310));
    CHECK_EQUAL(20, pc.update(300, 320));
}

TEST(packet_counter, wrap) {
    PacketCounter pc;

    CHECK_EQUAL(10, pc.update(0xFFFFFFFF - 30, 0xFFFFFFFF - 20));
    CHECK_EQUAL(20, pc.update(0xFFFFFFFF - 30, 0xFFFFFFFF - 10));
    CHECK_EQUAL(40, pc.update(0xFFFFFFFF - 30, 10));
    CHECK_EQUAL(60, pc.update(0xFFFFFFFF - 30, 30));
    CHECK_EQUAL(60, pc.update(0xFFFFFFFF - 30, 20));
    CHECK_EQUAL(70, pc.update(0xFFFFFFFF - 30, 40));

    CHECK_EQUAL(10, pc.update(10, 20));
}

} // namespace rtcp
} // namespace roc
