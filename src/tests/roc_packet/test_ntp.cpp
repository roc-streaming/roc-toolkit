/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/time.h"
#include "roc_packet/ntp.h"

namespace roc {
namespace packet {

TEST_GROUP(ntp) {};

TEST(ntp, ntp_2_unix) {
    CHECK_EQUAL(0, ntp_2_unix((uint64_t)2208988800ul << 32));
    CHECK_EQUAL((int64_t)1000 * 1000000000,
                ntp_2_unix((uint64_t)(2208988800ul + 1000) << 32));
    CHECK_EQUAL((int64_t)-1000 * 1000000000,
                ntp_2_unix((uint64_t)(2208988800ul - 1000) << 32));
}

TEST(ntp, unix_2_ntp) {
    CHECK_EQUAL(((uint64_t)2208988800ul << 32), unix_2_ntp(0));
    CHECK_EQUAL(((uint64_t)(2208988800ul + 1000) << 32),
                unix_2_ntp((int64_t)1000 * 1000000000));
    CHECK_EQUAL(((uint64_t)(2208988800ul - 1000) << 32),
                unix_2_ntp((int64_t)-1000 * 1000000000));
}

TEST(ntp, ntp_2_nanoseconds) {
    CHECK_EQUAL(0, ntp_2_nanoseconds(0));
    CHECK_EQUAL(1500 * core::Millisecond,
                ntp_2_nanoseconds(((uint64_t)1 << 31) + ((uint64_t)1 << 32)));
}

TEST(ntp, nanoseconds_2_ntp) {
    CHECK_EQUAL(ntp_timestamp_t(double(1e-9) * double((uint64_t)1 << 32)),
                nanoseconds_2_ntp(1));
    CHECK_EQUAL(((uint64_t)1 << 31) + ((uint64_t)1 << 32),
                nanoseconds_2_ntp(1500 * core::Millisecond));
}

} // namespace packet
} // namespace roc
