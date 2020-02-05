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

UdpSenderConfig make_sender_config(const char* ip, int port) {
    UdpSenderConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port));
    return config;
}

UdpReceiverConfig make_receiver_config(const char* ip, int port) {
    UdpReceiverConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port));
    return config;
}

} // namespace

TEST_GROUP(ports) {};

TEST(ports, init) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());
}

TEST(ports, add) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpSenderConfig tx_config = make_sender_config("0.0.0.0", 0);
    UdpReceiverConfig rx_config = make_receiver_config("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());

    EventLoop::PortHandle tx_handle = event_loop.add_udp_sender(tx_config, NULL);
    CHECK(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    EventLoop::PortHandle rx_handle = event_loop.add_udp_receiver(rx_config, queue);
    CHECK(rx_handle);
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());
}

TEST(ports, add_remove) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpSenderConfig tx_config = make_sender_config("0.0.0.0", 0);
    UdpReceiverConfig rx_config = make_receiver_config("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());

    EventLoop::PortHandle tx_handle = event_loop.add_udp_sender(tx_config, NULL);
    CHECK(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    EventLoop::PortHandle rx_handle = event_loop.add_udp_receiver(rx_config, queue);
    CHECK(rx_handle);
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    event_loop.remove_port(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    event_loop.remove_port(rx_handle);
    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());
}

TEST(ports, add_remove_add) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpSenderConfig tx_config = make_sender_config("0.0.0.0", 0);

    EventLoop::PortHandle tx_handle = event_loop.add_udp_sender(tx_config, NULL);
    CHECK(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    event_loop.remove_port(tx_handle);
    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());

    tx_handle = event_loop.add_udp_sender(tx_config, NULL);
    CHECK(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());
}

TEST(ports, add_duplicate) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpSenderConfig port1_tx = make_sender_config("0.0.0.0", 0);

    EventLoop::PortHandle tx_handle = event_loop.add_udp_sender(port1_tx, NULL);
    CHECK(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    UdpReceiverConfig port1_rx =
        make_receiver_config("0.0.0.0", port1_tx.bind_address.port());

    CHECK(!event_loop.add_udp_sender(port1_tx, NULL));
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    CHECK(!event_loop.add_udp_receiver(port1_rx, queue));
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    UdpReceiverConfig port2_rx = make_receiver_config("0.0.0.0", 0);

    EventLoop::PortHandle rx_handle = event_loop.add_udp_receiver(port2_rx, queue);
    CHECK(rx_handle);
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    UdpSenderConfig port2_tx =
        make_sender_config("0.0.0.0", port2_rx.bind_address.port());

    CHECK(!event_loop.add_udp_sender(port2_tx, NULL));
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    CHECK(!event_loop.add_udp_receiver(port2_rx, queue));
    UNSIGNED_LONGS_EQUAL(2, event_loop.num_ports());

    event_loop.remove_port(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    event_loop.remove_port(rx_handle);
    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());
}

} // namespace netio
} // namespace roc
