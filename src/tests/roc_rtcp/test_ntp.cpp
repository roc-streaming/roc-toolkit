/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/ntp.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

TEST_GROUP(ntp) {};

TEST(ntp, clamp) {
    // clamp 64
    {
        // no maximum
        CHECK_EQUAL(0xBBBBBBBBBBBBBBBB,
                    ntp_clamp_64(0xBBBBBBBBBBBBBBBB, 0xFFFFFFFFFFFFFFFF));

        // maximum
        CHECK_EQUAL(0x0000AAAAAAA90000,
                    ntp_clamp_64(0xBBBBBBBBBBBBBBBB, 0x0000AAAAAAA90000));
    }
    // clamp 32
    {
        // no maximum
        CHECK_EQUAL(0x0000AAAAAAAA0000,
                    ntp_clamp_32(0x0000AAAAAAAA0000, 0xFFFFFFFFFFFFFFFF));
        CHECK_EQUAL(0x0000AAAAAAAA0000,
                    ntp_clamp_32(0x1111AAAAAAAA7000, 0xFFFFFFFFFFFFFFFF));
        CHECK_EQUAL(0x0000AAAAAAAB0000,
                    ntp_clamp_32(0x1111AAAAAAAA8000, 0xFFFFFFFFFFFFFFFF));

        // maximum
        CHECK_EQUAL(0x0000AAAAAAA90000,
                    ntp_clamp_32(0x0000AAAAAAAA0000, 0x0000AAAAAAA90000));
        CHECK_EQUAL(0x0000AAAAAAA90000,
                    ntp_clamp_32(0x0000AAAAAAAA7000, 0x0000AAAAAAA90000));
        CHECK_EQUAL(0x0000AAAAAAA90000,
                    ntp_clamp_32(0x0000AAAAAAAA8000, 0x0000AAAAAAA90000));
        CHECK_EQUAL(0x0000AAAAAAA90000,
                    ntp_clamp_32(0xBBBBBBBBBBBBBBBB, 0x0000AAAAAAA90000));
    }
}

TEST(ntp, extend) {
    // time goes forward, no wrap
    {
        // middle 32 bits of original time are NOT close to wrap
        const packet::ntp_timestamp_t orig_time = 0xAAAA11112222AAAA;
        const packet::ntp_timestamp_t truncated_time = orig_time & 0x0000FFFFFFFF0000;
        // time went forward, wrap did not happen
        const packet::ntp_timestamp_t current_time = 0xAAAA33334444BBBB;

        // we can restore original time except last 16 bits
        CHECK_EQUAL(orig_time & 0xFFFFFFFFFFFF0000,
                    ntp_extend(current_time, truncated_time));
    }
    // time goes forward, wrap
    {
        // middle 32 bits of original time ARE close to wrap
        const packet::ntp_timestamp_t orig_time = 0xAAAAFFFFEEEEAAAA;
        const packet::ntp_timestamp_t truncated_time = orig_time & 0x0000FFFFFFFF0000;
        // time went forward, wrap happened
        const packet::ntp_timestamp_t current_time = 0xAAAB111122221111;

        // we can restore original time except last 16 bits
        CHECK_EQUAL(orig_time & 0xFFFFFFFFFFFF0000,
                    ntp_extend(current_time, truncated_time));
    }
    // time goes backward, no wrap
    {
        // middle 32 bits of original time are NOT close to backward wrap
        const packet::ntp_timestamp_t orig_time = 0xAAAAFFFFEEEEAAAA;
        const packet::ntp_timestamp_t truncated_time = orig_time & 0x0000FFFFFFFF0000;
        // time went backward, wrap did not happen
        const packet::ntp_timestamp_t current_time = 0xAAAADDDDCCCC1111;

        // we can restore original time except last 16 bits
        CHECK_EQUAL(orig_time & 0xFFFFFFFFFFFF0000,
                    ntp_extend(current_time, truncated_time));
    }
    // time goes backward, wrap
    {
        // middle 32 bits of original time ARE close to backward wrap
        const packet::ntp_timestamp_t orig_time = 0xAAAA11112222AAAA;
        const packet::ntp_timestamp_t truncated_time = orig_time & 0x0000FFFFFFFF0000;
        // time went backward, wrap happened
        const packet::ntp_timestamp_t current_time = 0xAAA9FFFFEEEEAAAA;

        // we can restore original time except last 16 bits
        CHECK_EQUAL(orig_time & 0xFFFFFFFFFFFF0000,
                    ntp_extend(current_time, truncated_time));
    }
}

} // namespace rtcp
} // namespace roc
