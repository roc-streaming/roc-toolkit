/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_netio/event_loop.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

} // namespace

TEST_GROUP(resolve) {};

TEST(resolve, ipv4) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    address::Endpoint endpoint(allocator);
    CHECK(address::parse_endpoint_uri("rtp://127.0.0.1:123",
                                      address::EndpointURI::Subset_Full, endpoint.uri()));

    address::SocketAddr address;
    CHECK(event_loop.resolve_endpoint_address(endpoint, address));

    LONGS_EQUAL(address::Family_IPv4, address.family());
    STRCMP_EQUAL("127.0.0.1:123", address::socket_addr_to_str(address).c_str());
}

TEST(resolve, ipv6) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    address::Endpoint endpoint(allocator);
    CHECK(address::parse_endpoint_uri("rtp://[::1]:123",
                                      address::EndpointURI::Subset_Full, endpoint.uri()));

    address::SocketAddr address;
    CHECK(event_loop.resolve_endpoint_address(endpoint, address));

    LONGS_EQUAL(address::Family_IPv6, address.family());
    STRCMP_EQUAL("[::1]:123", address::socket_addr_to_str(address).c_str());
}

TEST(resolve, hostname) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    address::Endpoint endpoint(allocator);
    CHECK(address::parse_endpoint_uri("rtp://localhost:123",
                                      address::EndpointURI::Subset_Full, endpoint.uri()));

    address::SocketAddr address;
    CHECK(event_loop.resolve_endpoint_address(endpoint, address));

    CHECK(address.family() == address::Family_IPv4
          || address.family() == address::Family_IPv6);

    if (address.family() == address::Family_IPv4) {
        STRCMP_EQUAL("127.0.0.1:123", address::socket_addr_to_str(address).c_str());
    } else {
        STRCMP_EQUAL("[::1]:123", address::socket_addr_to_str(address).c_str());
    }
}

TEST(resolve, default_port) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    address::Endpoint endpoint(allocator);
    CHECK(address::parse_endpoint_uri("rtsp://127.0.0.1",
                                      address::EndpointURI::Subset_Full, endpoint.uri()));

    address::SocketAddr address;
    CHECK(event_loop.resolve_endpoint_address(endpoint, address));

    STRCMP_EQUAL("127.0.0.1:554", address::socket_addr_to_str(address).c_str());
}

TEST(resolve, multicast) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    { // multicast interface not present
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri(
            "rtp://224.0.0.0:123", address::EndpointURI::Subset_Full, endpoint.uri()));

        address::SocketAddr address;
        CHECK(event_loop.resolve_endpoint_address(endpoint, address));

        CHECK(address.multicast());
        CHECK(!address.has_miface());
    }
    { // multicast interface present
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri(
            "rtp://224.0.0.0:123", address::EndpointURI::Subset_Full, endpoint.uri()));
        CHECK(endpoint.set_miface("192.168.0.1"));

        address::SocketAddr address;
        CHECK(event_loop.resolve_endpoint_address(endpoint, address));

        CHECK(address.multicast());
        CHECK(address.has_miface());

        char miface[address::SocketAddr::MaxStrLen];
        CHECK(address.get_miface(miface, sizeof(miface)));
        STRCMP_EQUAL("192.168.0.1", miface);
    }
    { // multicast interface present but address is not multicast
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri(
            "rtp://192.168.0.1:123", address::EndpointURI::Subset_Full, endpoint.uri()));
        CHECK(endpoint.set_miface("192.168.0.1"));

        address::SocketAddr address;
        CHECK(!event_loop.resolve_endpoint_address(endpoint, address));
    }
}

TEST(resolve, broadcast) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    { // broadcast not set
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri("rtp://223.255.255.255:123",
                                          address::EndpointURI::Subset_Full,
                                          endpoint.uri()));

        address::SocketAddr address;
        CHECK(event_loop.resolve_endpoint_address(endpoint, address));

        CHECK(!address.broadcast());
    }
    { // broadcast set
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri("rtp://223.255.255.255:123",
                                          address::EndpointURI::Subset_Full,
                                          endpoint.uri()));
        endpoint.set_broadcast(true);

        address::SocketAddr address;
        CHECK(event_loop.resolve_endpoint_address(endpoint, address));

        CHECK(address.broadcast());
    }
    { // broadcast set but address is multicast
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri(
            "rtp://224.0.0.0:123", address::EndpointURI::Subset_Full, endpoint.uri()));
        endpoint.set_broadcast(true);

        address::SocketAddr address;
        CHECK(!event_loop.resolve_endpoint_address(endpoint, address));
    }
}

TEST(resolve, bad_host) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    { // bad ipv4
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri(
            "rtp://300.0.0.1:123", address::EndpointURI::Subset_Full, endpoint.uri()));

        address::SocketAddr address;
        CHECK(!event_loop.resolve_endpoint_address(endpoint, address));
    }
    { // bad ipv6
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri(
            "rtp://[11::22::]:123", address::EndpointURI::Subset_Full, endpoint.uri()));

        address::SocketAddr address;
        CHECK(!event_loop.resolve_endpoint_address(endpoint, address));
    }
    { // bad hostname
        address::Endpoint endpoint(allocator);
        CHECK(address::parse_endpoint_uri(
            "rtp://_:123", address::EndpointURI::Subset_Full, endpoint.uri()));

        address::SocketAddr address;
        CHECK(!event_loop.resolve_endpoint_address(endpoint, address));
    }
}

} // namespace netio
} // namespace roc
