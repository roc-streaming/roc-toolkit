/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/ntp.h"

namespace roc {
namespace packet {

TEST_GROUP(ntp) {};

TEST(ntp, ntp_2_nanoseconds) {
    ntp_timestamp_t ntp = ((uint64_t)1 << 31) + ((uint64_t)1 << 32); // 1.5 seconds
    core::nanoseconds_t nans = ntp_2_nanoseconds(ntp);

    CHECK_EQUAL(1500 * core::Millisecond, nans);

    ntp = 0;
    nans = ntp_2_nanoseconds(ntp);
    CHECK_EQUAL(0, nans);
}

TEST(ntp, nanoseconds_2_ntp) {
    core::nanoseconds_t nans = 1500 * core::Millisecond;
    ntp_timestamp_t ntp = packet::nanoseconds_2_ntp(nans);

    CHECK_EQUAL(((uint64_t)1 << 31) + ((uint64_t)1 << 32), ntp);

    nans = 1;
    ntp = packet::nanoseconds_2_ntp(nans);
    CHECK_EQUAL(ntp_timestamp_t(double(1e-9) * double((uint64_t)1 << 32)), ntp);
}

} // namespace packet
} // namespace roc
