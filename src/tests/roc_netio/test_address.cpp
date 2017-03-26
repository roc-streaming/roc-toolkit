/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/log.h"
#include "roc_netio/inet_address.h"

namespace roc {
namespace test {

using namespace netio;
using namespace datagram;

TEST_GROUP(address) {
    LogLevel level;

    void setup() {
        level = core::set_log_level(LogNone);
    }

    void teardown() {
        core::set_log_level(level);
    }
};

TEST(address, regular) {
    Address addr;
    CHECK(parse_address("1.2.0.255:123", addr));

    LONGS_EQUAL(1, addr.ip[0]);
    LONGS_EQUAL(2, addr.ip[1]);
    LONGS_EQUAL(0, addr.ip[2]);
    LONGS_EQUAL(255, addr.ip[3]);
    LONGS_EQUAL(123, addr.port);
}

TEST(address, min_port) {
    Address addr;
    CHECK(parse_address("1.2.3.4:1", addr));

    LONGS_EQUAL(1, addr.ip[0]);
    LONGS_EQUAL(2, addr.ip[1]);
    LONGS_EQUAL(3, addr.ip[2]);
    LONGS_EQUAL(4, addr.ip[3]);
    LONGS_EQUAL(1, addr.port);
}

TEST(address, max_port) {
    Address addr;
    CHECK(parse_address("1.2.3.4:65535", addr));

    LONGS_EQUAL(1, addr.ip[0]);
    LONGS_EQUAL(2, addr.ip[1]);
    LONGS_EQUAL(3, addr.ip[2]);
    LONGS_EQUAL(4, addr.ip[3]);
    LONGS_EQUAL(65535, addr.port);
}

TEST(address, empty_ip) {
    Address addr;
    CHECK(parse_address(":123", addr));

    LONGS_EQUAL(0, addr.ip[0]);
    LONGS_EQUAL(0, addr.ip[1]);
    LONGS_EQUAL(0, addr.ip[2]);
    LONGS_EQUAL(0, addr.ip[3]);
    LONGS_EQUAL(123, addr.port);
}

TEST(address, bad_format) {
    Address addr;

    CHECK(!parse_address(NULL, addr));
    CHECK(!parse_address("", addr));
    CHECK(!parse_address("1.2.3.4", addr));
    CHECK(!parse_address("1.-2.3.4:123", addr));
    CHECK(!parse_address("1.a.3.4:123", addr));
    CHECK(!parse_address("1.2.3.4:", addr));
    CHECK(!parse_address("1.2.3.4:a", addr));
    CHECK(!parse_address("1 .2.3.4:123", addr));
    CHECK(!parse_address("1.2.3.4: 123", addr));
    CHECK(!parse_address("1.2.3.4:123 ", addr));
}

TEST(address, bad_range) {
    Address addr;

    CHECK(!parse_address(NULL, addr));
    CHECK(!parse_address("256.1.2.3:123", addr));
    CHECK(!parse_address("1.2.3.4:65536", addr));
    CHECK(!parse_address("1.2.3.4:0", addr));
    CHECK(!parse_address("1.2.3.4:-1", addr));
    CHECK(!parse_address("1.2.3.4:999999999999999", addr));
}

} // namespace test
} // namespace roc
