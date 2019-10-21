/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/address.h"
#include "roc_packet/address_to_str.h"

namespace roc {
namespace packet {

TEST_GROUP(address) {};

TEST(address, invalid) {
    Address addr;

    CHECK(!addr.has_host_port());

    UNSIGNED_LONGS_EQUAL(-1, addr.version());
    LONGS_EQUAL(-1, addr.port());

    STRCMP_EQUAL("<none>", address_to_str(addr).c_str());
}

TEST(address, set_ipv4) {
    Address addr;

    CHECK(addr.set_host_port_ipv4("1.2.0.255", 123));
    CHECK(addr.has_host_port());

    UNSIGNED_LONGS_EQUAL(4, addr.version());
    LONGS_EQUAL(123, addr.port());

    STRCMP_EQUAL("1.2.0.255:123", address_to_str(addr).c_str());
}

TEST(address, set_ipv6) {
    Address addr;

    CHECK(addr.set_host_port_ipv6("2001:db8::1", 123));
    CHECK(addr.has_host_port());

    UNSIGNED_LONGS_EQUAL(6, addr.version());
    LONGS_EQUAL(123, addr.port());

    STRCMP_EQUAL("[2001:db8::1]:123", address_to_str(addr).c_str());
}

TEST(address, get_ipv4) {
    Address addr;

    CHECK(addr.set_host_port_ipv4("1.2.0.255", 123));
    CHECK(addr.has_host_port());

    char buf[Address::MaxStrLen];
    CHECK(addr.get_host(buf, sizeof(buf)));

    STRCMP_EQUAL("1.2.0.255", buf);
}

TEST(address, get_ipv6) {
    Address addr;

    CHECK(addr.set_host_port_ipv6("2001:db8::1", 123));
    CHECK(addr.has_host_port());

    char buf[Address::MaxStrLen];
    CHECK(addr.get_host(buf, sizeof(buf)));

    STRCMP_EQUAL("2001:db8::1", buf);
}

TEST(address, eq_ipv4) {
    Address addr1;
    CHECK(addr1.set_host_port_ipv4("1.2.3.4", 123));
    CHECK(addr1.has_host_port());

    Address addr2;
    CHECK(addr2.set_host_port_ipv4("1.2.3.4", 123));
    CHECK(addr2.has_host_port());

    Address addr3;
    CHECK(addr3.set_host_port_ipv4("1.2.3.4", 456));
    CHECK(addr3.has_host_port());

    Address addr4;
    CHECK(addr4.set_host_port_ipv4("1.2.4.3", 123));
    CHECK(addr4.has_host_port());

    CHECK(addr1 == addr2);
    CHECK(!(addr1 == addr3));
    CHECK(!(addr1 == addr4));

    CHECK(!(addr1 != addr2));
    CHECK(addr1 != addr3);
    CHECK(addr1 != addr4);
}

TEST(address, eq_ipv4_multicast) {
    Address addr1;
    CHECK(addr1.set_host_port_ipv4("1.2.3.4", 123));
    CHECK(addr1.set_miface_ipv4("0.0.0.0"));
    CHECK(addr1.has_host_port());

    Address addr2;
    CHECK(addr2.set_host_port_ipv4("1.2.3.4", 123));
    CHECK(addr2.set_miface_ipv4("0.0.0.0"));
    CHECK(addr2.has_host_port());

    Address addr3;
    CHECK(addr3.set_host_port_ipv4("1.2.3.4", 123));
    CHECK(addr3.set_miface_ipv4("0.0.0.1"));
    CHECK(addr3.has_host_port());

    CHECK(addr1 == addr1);

    CHECK(addr1 == addr2);
    CHECK(!(addr1 != addr2));

    CHECK(addr1 != addr3);
    CHECK(addr2 != addr3);
    CHECK(!(addr1 == addr3));
    CHECK(!(addr2 == addr3));
}

TEST(address, eq_ipv6) {
    Address addr1;
    CHECK(addr1.set_host_port_ipv6("2001:db1::1", 123));
    CHECK(addr1.has_host_port());

    Address addr2;
    CHECK(addr2.set_host_port_ipv6("2001:db1::1", 123));
    CHECK(addr2.has_host_port());

    Address addr3;
    CHECK(addr3.set_host_port_ipv6("2001:db1::1", 456));
    CHECK(addr3.has_host_port());

    Address addr4;
    CHECK(addr4.set_host_port_ipv6("2001:db2::1", 123));
    CHECK(addr4.has_host_port());

    CHECK(addr1 == addr2);
    CHECK(!(addr1 == addr3));
    CHECK(!(addr1 == addr4));

    CHECK(!(addr1 != addr2));
    CHECK(addr1 != addr3);
    CHECK(addr1 != addr4);
}

TEST(address, eq_ipv6_multicast) {
    Address addr1;
    CHECK(addr1.set_host_port_ipv6("ffee::", 123));
    CHECK(addr1.set_miface_ipv6("::"));
    CHECK(addr1.has_host_port());
    CHECK(addr1.multicast());

    Address addr2;
    CHECK(addr2.set_host_port_ipv6("ffee::", 123));
    CHECK(addr2.set_miface_ipv6("::"));
    CHECK(addr2.has_host_port());
    CHECK(addr1.multicast());

    Address addr3;
    CHECK(addr3.set_host_port_ipv6("ffee::", 123));
    CHECK(addr3.set_miface_ipv6("2001:db8::1"));
    CHECK(addr3.has_host_port());
    CHECK(addr1.multicast());

    CHECK(addr1 == addr1);

    CHECK(addr1 == addr2);
    CHECK(!(addr1 != addr2));

    CHECK(addr1 != addr3);
    CHECK(addr2 != addr3);
    CHECK(!(addr1 == addr3));
    CHECK(!(addr2 == addr3));
}

TEST(address, multicast_ipv4) {
    {
        Address addr;
        CHECK(addr.set_host_port_ipv4("223.255.255.255", 123));
        CHECK(addr.has_host_port());
        CHECK(!addr.multicast());
    }

    {
        Address addr;
        CHECK(addr.set_host_port_ipv4("224.0.0.0", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        Address addr;
        CHECK(addr.set_host_port_ipv4("227.128.128.128", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        Address addr;
        CHECK(addr.set_host_port_ipv4("239.255.255.255", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        Address addr;
        CHECK(addr.set_host_port_ipv4("240.0.0.0", 123));
        CHECK(addr.has_host_port());
        CHECK(!addr.multicast());
    }
}

TEST(address, multicast_ipv6) {
    {
        Address addr;
        CHECK(addr.set_host_port_ipv6("fe00::", 123));
        CHECK(addr.has_host_port());
        CHECK(!addr.multicast());
    }

    {
        Address addr;
        CHECK(addr.set_host_port_ipv6("ff00::", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        Address addr;
        CHECK(addr.set_host_port_ipv6("ff11:1:1:1:1:1:1:1", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        Address addr;
        CHECK(addr.set_host_port_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }
}

TEST(address, multicast_ipv4_to_str) {
    {
        Address addr;

        CHECK(addr.set_host_port_ipv4("239.255.255.255", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());

        STRCMP_EQUAL("239.255.255.255:123", address_to_str(addr).c_str());
    }
    {
        Address addr;

        CHECK(addr.set_host_port_ipv4("239.255.255.255", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());

        CHECK(addr.set_miface_ipv4("0.0.0.0"));

        STRCMP_EQUAL("239.255.255.255:123 miface 0.0.0.0", address_to_str(addr).c_str());
    }
}

TEST(address, multicast_ipv6_to_str) {
    {
        Address addr;

        CHECK(addr.set_host_port_ipv6("ff00::", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());

        STRCMP_EQUAL("[ff00::]:123", address_to_str(addr).c_str());
    }
    {
        Address addr;

        CHECK(addr.set_host_port_ipv6("ff00::", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());

        CHECK(addr.set_miface_ipv6("::"));

        STRCMP_EQUAL("[ff00::]:123 miface [::]", address_to_str(addr).c_str());
    }
}

} // namespace packet
} // namespace roc
