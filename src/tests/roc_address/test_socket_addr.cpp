/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_address/socket_addr_to_str.h"

namespace roc {
namespace address {

TEST_GROUP(socket_addr) {};

TEST(socket_addr, empty) {
    SocketAddr addr;

    CHECK(!addr.has_host_port());

    UNSIGNED_LONGS_EQUAL(Family_Unknown, addr.family());
    LONGS_EQUAL(-1, addr.port());

    STRCMP_EQUAL("<none>", socket_addr_to_str(addr).c_str());
}

TEST(socket_addr, set_ipv4) {
    SocketAddr addr;

    CHECK(addr.set_host_port(Family_IPv4, "1.2.0.255", 123));
    CHECK(addr.has_host_port());

    UNSIGNED_LONGS_EQUAL(Family_IPv4, addr.family());
    LONGS_EQUAL(123, addr.port());

    STRCMP_EQUAL("1.2.0.255:123", socket_addr_to_str(addr).c_str());
}

TEST(socket_addr, set_ipv6) {
    SocketAddr addr;

    CHECK(addr.set_host_port(Family_IPv6, "2001:db8::1", 123));
    CHECK(addr.has_host_port());

    UNSIGNED_LONGS_EQUAL(Family_IPv6, addr.family());
    LONGS_EQUAL(123, addr.port());

    STRCMP_EQUAL("[2001:db8::1]:123", socket_addr_to_str(addr).c_str());
}

TEST(socket_addr, set_auto) {
    {
        SocketAddr addr;

        CHECK(addr.set_host_port_auto("1.2.0.255", 123));
        CHECK(addr.has_host_port());

        UNSIGNED_LONGS_EQUAL(Family_IPv4, addr.family());
        LONGS_EQUAL(123, addr.port());

        STRCMP_EQUAL("1.2.0.255:123", socket_addr_to_str(addr).c_str());
    }
    {
        SocketAddr addr;

        CHECK(addr.set_host_port_auto("2001:db8::1", 123));
        CHECK(addr.has_host_port());

        UNSIGNED_LONGS_EQUAL(Family_IPv6, addr.family());
        LONGS_EQUAL(123, addr.port());

        STRCMP_EQUAL("[2001:db8::1]:123", socket_addr_to_str(addr).c_str());
    }
}

TEST(socket_addr, get_ipv4) {
    SocketAddr addr;

    CHECK(addr.set_host_port(Family_IPv4, "1.2.0.255", 123));
    CHECK(addr.has_host_port());

    char buf[SocketAddr::MaxStrLen];
    CHECK(addr.get_host(buf, sizeof(buf)));

    STRCMP_EQUAL("1.2.0.255", buf);
}

TEST(socket_addr, get_ipv6) {
    SocketAddr addr;

    CHECK(addr.set_host_port(Family_IPv6, "2001:db8::1", 123));
    CHECK(addr.has_host_port());

    char buf[SocketAddr::MaxStrLen];
    CHECK(addr.get_host(buf, sizeof(buf)));

    STRCMP_EQUAL("2001:db8::1", buf);
}

TEST(socket_addr, eq_ipv4) {
    SocketAddr addr1;
    CHECK(addr1.set_host_port(Family_IPv4, "1.2.3.4", 123));
    CHECK(addr1.has_host_port());

    SocketAddr addr2;
    CHECK(addr2.set_host_port(Family_IPv4, "1.2.3.4", 123));
    CHECK(addr2.has_host_port());

    SocketAddr addr3;
    CHECK(addr3.set_host_port(Family_IPv4, "1.2.3.4", 456));
    CHECK(addr3.has_host_port());

    SocketAddr addr4;
    CHECK(addr4.set_host_port(Family_IPv4, "1.2.4.3", 123));
    CHECK(addr4.has_host_port());

    CHECK(addr1 == addr2);
    CHECK(!(addr1 == addr3));
    CHECK(!(addr1 == addr4));

    CHECK(!(addr1 != addr2));
    CHECK(addr1 != addr3);
    CHECK(addr1 != addr4);
}

TEST(socket_addr, eq_ipv6) {
    SocketAddr addr1;
    CHECK(addr1.set_host_port(Family_IPv6, "2001:db1::1", 123));
    CHECK(addr1.has_host_port());

    SocketAddr addr2;
    CHECK(addr2.set_host_port(Family_IPv6, "2001:db1::1", 123));
    CHECK(addr2.has_host_port());

    SocketAddr addr3;
    CHECK(addr3.set_host_port(Family_IPv6, "2001:db1::1", 456));
    CHECK(addr3.has_host_port());

    SocketAddr addr4;
    CHECK(addr4.set_host_port(Family_IPv6, "2001:db2::1", 123));
    CHECK(addr4.has_host_port());

    CHECK(addr1 == addr2);
    CHECK(!(addr1 == addr3));
    CHECK(!(addr1 == addr4));

    CHECK(!(addr1 != addr2));
    CHECK(addr1 != addr3);
    CHECK(addr1 != addr4);
}

TEST(socket_addr, multicast_ipv4) {
    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv4, "223.255.255.255", 123));
        CHECK(addr.has_host_port());
        CHECK(!addr.multicast());
    }

    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv4, "224.0.0.0", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv4, "227.128.128.128", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv4, "239.255.255.255", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv4, "240.0.0.0", 123));
        CHECK(addr.has_host_port());
        CHECK(!addr.multicast());
    }
}

TEST(socket_addr, multicast_ipv6) {
    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv6, "fe00::", 123));
        CHECK(addr.has_host_port());
        CHECK(!addr.multicast());
    }

    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv6, "ff00::", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv6, "ff11:1:1:1:1:1:1:1", 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }

    {
        SocketAddr addr;
        CHECK(addr.set_host_port(Family_IPv6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
                                 123));
        CHECK(addr.has_host_port());
        CHECK(addr.multicast());
    }
}

TEST(socket_addr, clear) {
    SocketAddr addr;
    CHECK(addr.set_host_port(Family_IPv4, "239.255.255.255", 123));

    CHECK(addr != SocketAddr());
    addr.clear();
    CHECK(addr == SocketAddr());
}

} // namespace address
} // namespace roc
