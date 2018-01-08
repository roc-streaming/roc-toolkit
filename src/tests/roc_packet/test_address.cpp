/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/address_to_str.h"
#include "roc_packet/parse_address.h"

namespace roc {
namespace packet {

TEST_GROUP(address){};

TEST(address, invalid) {
    Address addr;
    STRCMP_EQUAL("", address_to_str(addr).c_str());
}

TEST(address, ipv4) {
    Address addr;
    CHECK(parse_address("1.2.0.255:123", addr));
    STRCMP_EQUAL("1.2.0.255:123", address_to_str(addr).c_str());
}

TEST(address, ipv6) {
    Address addr;
    CHECK(parse_address("[2001:db8::1]:123", addr));
    STRCMP_EQUAL("[2001:db8::1]:123", address_to_str(addr).c_str());
}

TEST(address, port) {
    Address addr1;
    LONGS_EQUAL(-1, addr1.port());

    Address addr2;
    CHECK(parse_address("1.2.3.4:0", addr2));
    LONGS_EQUAL(0, addr2.port());

    Address addr3;
    CHECK(parse_address("1.2.3.4:123", addr3));
    LONGS_EQUAL(123, addr3.port());
}

TEST(address, eq) {
    Address addr1;
    CHECK(parse_address("1.2.3.4:123", addr1));

    Address addr2;
    CHECK(parse_address("1.2.3.4:123", addr2));

    Address addr3;
    CHECK(parse_address("1.2.3.4:456", addr3));

    Address addr4;
    CHECK(parse_address("1.2.4.3:123", addr4));

    CHECK(addr1 == addr2);
    CHECK(!(addr1 == addr3));
    CHECK(!(addr1 == addr4));

    CHECK(!(addr1 != addr2));
    CHECK(addr1 != addr3);
    CHECK(addr1 != addr4);
}

TEST(address, min_port) {
    Address addr;
    CHECK(parse_address("1.2.3.4:0", addr));
    STRCMP_EQUAL("1.2.3.4:0", address_to_str(addr).c_str());
}

TEST(address, max_port) {
    Address addr;
    CHECK(parse_address("1.2.3.4:65535", addr));
    STRCMP_EQUAL("1.2.3.4:65535", address_to_str(addr).c_str());
}

TEST(address, zero) {
    Address addr;
    CHECK(parse_address("0.0.0.0:0", addr));
    STRCMP_EQUAL("0.0.0.0:0", address_to_str(addr).c_str());
}

TEST(address, empty_ip) {
    Address addr;
    CHECK(parse_address(":123", addr));
    STRCMP_EQUAL("0.0.0.0:123", address_to_str(addr).c_str());
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
    CHECK(!parse_address("1.2.3.4:-1", addr));
    CHECK(!parse_address("1.2.3.4:999999999999999", addr));
}

} // namespace packet
} // namespace roc
