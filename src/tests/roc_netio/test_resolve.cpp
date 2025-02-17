/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_netio/network_loop.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapArena arena;
core::SlabPool<core::Buffer> buffer_pool("buffer_pool", arena, MaxBufSize);
core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);

bool resolve_endpoint_address(NetworkLoop& net_loop,
                              const address::NetworkUri& endpoint_uri,
                              address::SocketAddr& result_address) {
    NetworkLoop::Tasks::ResolveEndpointAddress task(endpoint_uri);
    CHECK(!task.success());
    if (!net_loop.schedule_and_wait(task)) {
        CHECK(!task.success());
        return false;
    }
    CHECK(task.success());
    result_address = task.get_address();
    return true;
}

} // namespace

TEST_GROUP(resolve) {};

TEST(resolve, ipv4) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    address::NetworkUri endpoint_uri(arena);
    CHECK(address::parse_network_uri("rtp://127.0.0.1:123",
                                     address::NetworkUri::Subset_Full, endpoint_uri));

    address::SocketAddr address;
    CHECK(resolve_endpoint_address(net_loop, endpoint_uri, address));

    LONGS_EQUAL(address::Family_IPv4, address.family());
    STRCMP_EQUAL("127.0.0.1:123", address::socket_addr_to_str(address).c_str());
}

TEST(resolve, ipv6) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    address::NetworkUri endpoint_uri(arena);
    CHECK(address::parse_network_uri("rtp://[::1]:123", address::NetworkUri::Subset_Full,
                                     endpoint_uri));

    address::SocketAddr address;
    CHECK(resolve_endpoint_address(net_loop, endpoint_uri, address));

    LONGS_EQUAL(address::Family_IPv6, address.family());
    STRCMP_EQUAL("[::1]:123", address::socket_addr_to_str(address).c_str());
}

TEST(resolve, hostname) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    address::NetworkUri endpoint_uri(arena);
    CHECK(address::parse_network_uri("rtp://localhost:123",
                                     address::NetworkUri::Subset_Full, endpoint_uri));

    address::SocketAddr address;
    CHECK(resolve_endpoint_address(net_loop, endpoint_uri, address));

    CHECK(address.family() == address::Family_IPv4
          || address.family() == address::Family_IPv6);

    if (address.family() == address::Family_IPv4) {
        STRCMP_EQUAL("127.0.0.1:123", address::socket_addr_to_str(address).c_str());
    } else {
        STRCMP_EQUAL("[::1]:123", address::socket_addr_to_str(address).c_str());
    }
}

TEST(resolve, standard_port) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    address::NetworkUri endpoint_uri(arena);
    CHECK(address::parse_network_uri("rtsp://127.0.0.1", address::NetworkUri::Subset_Full,
                                     endpoint_uri));

    address::SocketAddr address;
    CHECK(resolve_endpoint_address(net_loop, endpoint_uri, address));

    STRCMP_EQUAL("127.0.0.1:554", address::socket_addr_to_str(address).c_str());
}

TEST(resolve, bad_host) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    { // bad ipv4
        address::NetworkUri endpoint_uri(arena);
        CHECK(address::parse_network_uri("rtp://300.0.0.1:123",
                                         address::NetworkUri::Subset_Full, endpoint_uri));

        address::SocketAddr address;
        CHECK(!resolve_endpoint_address(net_loop, endpoint_uri, address));
    }
    { // bad ipv6
        address::NetworkUri endpoint_uri(arena);
        CHECK(address::parse_network_uri("rtp://[11::22::]:123",
                                         address::NetworkUri::Subset_Full, endpoint_uri));

        address::SocketAddr address;
        CHECK(!resolve_endpoint_address(net_loop, endpoint_uri, address));
    }
    { // bad hostname
        address::NetworkUri endpoint_uri(arena);
        CHECK(address::parse_network_uri("rtp://_:123", address::NetworkUri::Subset_Full,
                                         endpoint_uri));

        address::SocketAddr address;
        CHECK(!resolve_endpoint_address(net_loop, endpoint_uri, address));
    }
}

} // namespace netio
} // namespace roc
