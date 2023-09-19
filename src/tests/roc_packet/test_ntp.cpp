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
    // unix epoch
    CHECK_EQUAL(0, //
                ntp_2_unix(uint64_t(2208988800ul) << 32));

    // unix epoch + 1000 seconds
    CHECK_EQUAL(1000 * core::Second, //
                ntp_2_unix(uint64_t(2208988800ul + 1000) << 32));

    // unix epoch - 1000 seconds
    CHECK_EQUAL(-1000 * core::Second, //
                ntp_2_unix(uint64_t(2208988800ul - 1000) << 32));

    // era1
    CHECK_EQUAL(2085978496ul * core::Second, //
                ntp_2_unix(0));

    // era1 + 1000 seconds
    CHECK_EQUAL((2085978496ul + 1000) * core::Second, //
                ntp_2_unix(uint64_t(1000) << 32));

    // era1 - 1000 seconds
    CHECK_EQUAL((2085978496ul - 1000) * core::Second, //
                ntp_2_unix(uint64_t(2208988800ul + 2085978496ul - 1000) << 32));
}

TEST(ntp, unix_2_ntp) {
    // unix epoch
    CHECK_EQUAL(uint64_t(2208988800ul) << 32, //
                unix_2_ntp(0));

    // unix epoch + 1000 seconds
    CHECK_EQUAL(uint64_t(2208988800ul + 1000) << 32, //
                unix_2_ntp(1000 * core::Second));

    // unix epoch - 1000 seconds
    CHECK_EQUAL(uint64_t(2208988800ul - 1000) << 32, //
                unix_2_ntp(-1000 * core::Second));

    // era1
    CHECK_EQUAL(0, //
                unix_2_ntp(2085978496ul * core::Second));

    // era1 + 1000 seconds
    CHECK_EQUAL(uint64_t(1000) << 32, //
                unix_2_ntp((2085978496ul + 1000) * core::Second));

    // era1 - 1000 seconds
    CHECK_EQUAL(uint64_t(2208988800ul + 2085978496ul - 1000) << 32, //
                unix_2_ntp((2085978496ul - 1000) * core::Second));
}

TEST(ntp, ntp_2_unix_2_ntp) {
    // unix epoch
    CHECK_EQUAL(uint64_t(2208988800ul) << 32, //
                unix_2_ntp(ntp_2_unix(uint64_t(2208988800ul) << 32)));

    // unix epoch + 1000 seconds
    CHECK_EQUAL(uint64_t(2208988800ul + 1000) << 32, //
                unix_2_ntp(ntp_2_unix(uint64_t(2208988800ul + 1000) << 32)));

    // unix epoch - 1000 seconds
    CHECK_EQUAL(uint64_t(2208988800ul - 1000) << 32, //
                unix_2_ntp(ntp_2_unix(uint64_t(2208988800ul - 1000) << 32)));

    // era1
    CHECK_EQUAL(0, //
                unix_2_ntp(ntp_2_unix(0)));

    // era1 + 1000 seconds
    CHECK_EQUAL(uint64_t(1000) << 32, //
                unix_2_ntp(ntp_2_unix(uint64_t(1000) << 32)));

    // era1 - 1000 seconds
    CHECK_EQUAL(
        uint64_t(2208988800ul + 2085978496ul - 1000) << 32, //
        unix_2_ntp(ntp_2_unix(uint64_t(2208988800ul + 2085978496ul - 1000) << 32)));
}

TEST(ntp, unix_2_ntp_2_unix) {
    // unix epoch
    CHECK_EQUAL(0, //
                ntp_2_unix(unix_2_ntp(0)));

    // unix epoch + 1000 seconds
    CHECK_EQUAL(1000 * core::Second, //
                ntp_2_unix(unix_2_ntp(1000 * core::Second)));

    // unix epoch - 1000 seconds
    CHECK_EQUAL(-1000 * core::Second, //
                ntp_2_unix(unix_2_ntp(-1000 * core::Second)));

    // era1
    CHECK_EQUAL(2085978496ul * core::Second, //
                ntp_2_unix(unix_2_ntp(2085978496ul * core::Second)));

    // era1 + 1000 seconds
    CHECK_EQUAL((2085978496ul + 1000) * core::Second, //
                ntp_2_unix(unix_2_ntp((2085978496ul + 1000) * core::Second)));

    // era1 - 1000 seconds
    CHECK_EQUAL((2085978496ul - 1000) * core::Second, //
                ntp_2_unix(unix_2_ntp((2085978496ul - 1000) * core::Second)));
}

TEST(ntp, ntp_2_nanoseconds) {
    // 0ns
    CHECK_EQUAL(0, ntp_2_nanoseconds(0));

    // 1ns
    CHECK_EQUAL(0, ntp_2_nanoseconds(1));

    // 1500ms
    CHECK_EQUAL(1500 * core::Millisecond,
                ntp_2_nanoseconds(((uint64_t)1 << 31) + ((uint64_t)1 << 32)));
}

TEST(ntp, nanoseconds_2_ntp) {
    // 0ns
    CHECK_EQUAL(0, nanoseconds_2_ntp(0));

    // 1ns
    CHECK_EQUAL(ntp_timestamp_t(double(1e-9) * double((uint64_t)1 << 32)),
                nanoseconds_2_ntp(1));

    // 1500ms
    CHECK_EQUAL(((uint64_t)1 << 31) + ((uint64_t)1 << 32),
                nanoseconds_2_ntp(1500 * core::Millisecond));
}

} // namespace packet
} // namespace roc
