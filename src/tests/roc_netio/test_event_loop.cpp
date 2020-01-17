/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_netio/event_loop.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

address::SocketAddr make_address(const char* ip, int port) {
    address::SocketAddr addr;
    CHECK(addr.set_host_port(address::Family_IPv4, ip, port));
    return addr;
}

} // namespace

TEST_GROUP(transceiver) {};

TEST(transceiver, init) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);

    CHECK(event_loop.valid());
}

TEST(transceiver, bind_any) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);

    CHECK(event_loop.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    CHECK(event_loop.add_udp_sender(tx_addr));
    CHECK(event_loop.add_udp_receiver(rx_addr, queue));

    event_loop.remove_port(tx_addr);
    event_loop.remove_port(rx_addr);
}

TEST(transceiver, bind_lo) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);

    CHECK(event_loop.valid());

    address::SocketAddr tx_addr = make_address("127.0.0.1", 0);
    address::SocketAddr rx_addr = make_address("127.0.0.1", 0);

    CHECK(event_loop.add_udp_sender(tx_addr));
    CHECK(event_loop.add_udp_receiver(rx_addr, queue));

    event_loop.remove_port(tx_addr);
    event_loop.remove_port(rx_addr);
}

TEST(transceiver, bind_addrinuse) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop1(packet_pool, buffer_pool, allocator);
    CHECK(event_loop1.valid());

    address::SocketAddr tx_addr = make_address("127.0.0.1", 0);
    address::SocketAddr rx_addr = make_address("127.0.0.1", 0);

    CHECK(event_loop1.add_udp_sender(tx_addr));
    CHECK(event_loop1.add_udp_receiver(rx_addr, queue));

    EventLoop event_loop2(packet_pool, buffer_pool, allocator);
    CHECK(event_loop2.valid());

    CHECK(!event_loop2.add_udp_sender(tx_addr));
    CHECK(!event_loop2.add_udp_receiver(rx_addr, queue));
}

TEST(transceiver, add) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);

    CHECK(event_loop.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    CHECK(event_loop.add_udp_sender(tx_addr));
    CHECK(event_loop.add_udp_receiver(rx_addr, queue));
}

TEST(transceiver, add_remove) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);

    CHECK(event_loop.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());

    CHECK(event_loop.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    CHECK(event_loop.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    event_loop.remove_port(tx_addr);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    event_loop.remove_port(rx_addr);
    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());
}

TEST(transceiver, add_remove_add) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);

    CHECK(event_loop.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);

    CHECK(event_loop.add_udp_sender(tx_addr));
    event_loop.remove_port(tx_addr);
    CHECK(event_loop.add_udp_sender(tx_addr));
}

TEST(transceiver, add_duplicate) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);

    CHECK(event_loop.valid());

    address::SocketAddr tx_addr = make_address("0.0.0.0", 0);
    address::SocketAddr rx_addr = make_address("0.0.0.0", 0);

    CHECK(event_loop.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    CHECK(!event_loop.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    CHECK(!event_loop.add_udp_receiver(tx_addr, queue));
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    CHECK(event_loop.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    CHECK(!event_loop.add_udp_sender(rx_addr));
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    CHECK(!event_loop.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    event_loop.remove_port(tx_addr);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    event_loop.remove_port(rx_addr);
    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());
}

} // namespace netio
} // namespace roc
