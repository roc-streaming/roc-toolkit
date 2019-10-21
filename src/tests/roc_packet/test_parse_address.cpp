/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/parse_address.h"

namespace roc {
namespace packet {

TEST_GROUP(parse_address) {};

TEST(parse_address, miface_ipv4) {
    Address addr;

    CHECK(addr.set_host_port_ipv4("225.1.2.3", 123));
    CHECK(addr.has_host_port());
    CHECK(addr.multicast());

    CHECK(set_miface_from_string("0.0.0.0", addr));
    CHECK(addr.has_miface());

    char miface[64];
    CHECK(addr.get_miface(miface, sizeof(miface)));
    STRCMP_EQUAL("0.0.0.0", miface);
}

TEST(parse_address, miface_ipv6) {
    Address addr;

    CHECK(addr.set_host_port_ipv6("ffaa::", 123));
    CHECK(addr.has_host_port());
    CHECK(addr.multicast());

    CHECK(set_miface_from_string("[2001:db8::1]", addr));
    CHECK(addr.has_miface());

    char miface[64];
    CHECK(addr.get_miface(miface, sizeof(miface)));
    STRCMP_EQUAL("2001:db8::1", miface);
}

TEST(parse_address, bad_miface) {
    { // invalid address
        Address addr;
        CHECK(!set_miface_from_string("0.0.0.0", addr));
    }
    { // non-multicast address
        Address addr;

        CHECK(addr.set_host_port_ipv6("2001:db8::1", 123));
        CHECK(addr.has_host_port());
        CHECK(!addr.multicast());

        CHECK(!set_miface_from_string("[::]", addr));
    }
    { // empty miface
        Address addr;

        CHECK(addr.set_host_port_ipv4("225.1.2.3", 123));
        CHECK(addr.has_host_port());

        CHECK(!set_miface_from_string("", addr));
        CHECK(!set_miface_from_string(NULL, addr));
    }
    { // ipv6 miface for ipv4 addr
        Address addr;

        CHECK(addr.set_host_port_ipv4("225.1.2.3", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());

        CHECK(!set_miface_from_string("[::]", addr));
    }
    { // ipv4 miface for ipv6 addr
        Address addr;

        CHECK(addr.set_host_port_ipv6("ffaa::", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());

        CHECK(!set_miface_from_string("0.0.0.0", addr));
    }
}

} // namespace packet
} // namespace roc
