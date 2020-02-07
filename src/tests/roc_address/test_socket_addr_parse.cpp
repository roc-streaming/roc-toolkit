/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/parse_socket_addr.h"

namespace roc {
namespace address {

TEST_GROUP(socket_addr_parse) {};

TEST(socket_addr_parse, host_port_ipv4) {
    SocketAddr addr;

    CHECK(parse_socket_addr("0.0.0.0", 123, addr));
    CHECK(addr.has_host_port());

    char host[64];
    CHECK(addr.get_host(host, sizeof(host)));
    STRCMP_EQUAL("0.0.0.0", host);
    LONGS_EQUAL(Family_IPv4, addr.family());
    LONGS_EQUAL(123, addr.port());
}

TEST(socket_addr_parse, host_port_ipv6) {
    SocketAddr addr;

    CHECK(parse_socket_addr("[11::]", 123, addr));
    CHECK(addr.has_host_port());

    char host[64];
    CHECK(addr.get_host(host, sizeof(host)));
    STRCMP_EQUAL("11::", host);
    LONGS_EQUAL(Family_IPv6, addr.family());
    LONGS_EQUAL(123, addr.port());
}

TEST(socket_addr_parse, bad_host_port) {
    { // invalid port
        SocketAddr addr;
        CHECK(!parse_socket_addr("1.1.1.1", -3, addr));
    }
    { // invalid host
        SocketAddr addr;
        CHECK(!parse_socket_addr("", 123, addr));
        CHECK(!parse_socket_addr("abc.com", 123, addr));
        CHECK(!parse_socket_addr("1.2", 123, addr));
        CHECK(!parse_socket_addr("[11::", 123, addr));
        CHECK(!parse_socket_addr("11::]", 123, addr));
    }
}

} // namespace address
} // namespace roc
