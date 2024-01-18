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

TEST_GROUP(headers) {};

TEST(headers, extend_timestamp) {
    { // no wrap
        const packet::ntp_timestamp_t base = 0xAAAABBBBCCCCDDDD;
        const packet::ntp_timestamp_t value = 0x0000CCCCDDDD0000;

        CHECK_EQUAL(0xAAAACCCCDDDD0000, header::extend_timestamp(base, value));
    }
    { // wrap
        const packet::ntp_timestamp_t base = 0xAAAABBBBCCCCDDDD;
        const packet::ntp_timestamp_t value = 0x0000111122220000;

        CHECK_EQUAL(0xAAAB111122220000, header::extend_timestamp(base, value));
    }
}

} // namespace rtcp
} // namespace roc
