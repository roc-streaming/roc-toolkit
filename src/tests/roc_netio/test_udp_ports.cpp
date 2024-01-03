/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_netio/network_loop.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapArena arena;
core::BufferFactory<uint8_t> buffer_factory(arena, MaxBufSize);
packet::PacketFactory packet_factory(arena);

UdpConfig make_udp_config(const char* ip, int port) {
    UdpConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port)
          || config.bind_address.set_host_port(address::Family_IPv6, ip, port));
    return config;
}

NetworkLoop::PortHandle add_udp_sender(NetworkLoop& net_loop, UdpConfig& config) {
    NetworkLoop::Tasks::AddUdpPort task(config, netio::UdpSend, NULL);
    CHECK(!task.success());
    if (!net_loop.schedule_and_wait(task)) {
        CHECK(!task.success());
        return NULL;
    }
    CHECK(task.success());
    CHECK(task.get_outbound_writer());
    return task.get_handle();
}

NetworkLoop::PortHandle add_udp_receiver(NetworkLoop& net_loop,
                                         UdpConfig& config,
                                         packet::IWriter& inbound_writer) {
    NetworkLoop::Tasks::AddUdpPort task(config, netio::UdpRecv, &inbound_writer);
    CHECK(!task.success());
    if (!net_loop.schedule_and_wait(task)) {
        CHECK(!task.success());
        return NULL;
    }
    CHECK(task.success());
    CHECK(!task.get_outbound_writer());
    return task.get_handle();
}

NetworkLoop::PortHandle add_udp_sender_receiver(NetworkLoop& net_loop,
                                                UdpConfig& config,
                                                packet::IWriter& inbound_writer) {
    NetworkLoop::Tasks::AddUdpPort task(config, netio::UdpSendRecv, &inbound_writer);
    CHECK(!task.success());
    if (!net_loop.schedule_and_wait(task)) {
        CHECK(!task.success());
        return NULL;
    }
    CHECK(task.success());
    CHECK(task.get_outbound_writer());
    return task.get_handle();
}

void remove_port(NetworkLoop& net_loop, NetworkLoop::PortHandle handle) {
    NetworkLoop::Tasks::RemovePort task(handle);
    CHECK(!task.success());
    CHECK(net_loop.schedule_and_wait(task));
    CHECK(task.success());
}

} // namespace

TEST_GROUP(udp_ports) {};

TEST(udp_ports, no_ports) {
    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(udp_ports, add_anyaddr) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);
    UdpConfig rx_config = make_udp_config("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle tx_handle = add_udp_sender(net_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::PortHandle rx_handle = add_udp_receiver(net_loop, rx_config, queue);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());
}

TEST(udp_ports, add_localhost) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UdpConfig tx_config = make_udp_config("127.0.0.1", 0);
    UdpConfig rx_config = make_udp_config("127.0.0.1", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle tx_handle = add_udp_sender(net_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::PortHandle rx_handle = add_udp_receiver(net_loop, rx_config, queue);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());
}

TEST(udp_ports, add_addrinuse) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop1(packet_factory, buffer_factory, arena);
    CHECK(net_loop1.is_valid());

    UdpConfig tx_config = make_udp_config("127.0.0.1", 0);
    UdpConfig rx_config = make_udp_config("127.0.0.1", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop1.num_ports());

    NetworkLoop::PortHandle tx_handle = add_udp_sender(net_loop1, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop1.num_ports());

    NetworkLoop::PortHandle rx_handle = add_udp_receiver(net_loop1, rx_config, queue);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(2, net_loop1.num_ports());

    NetworkLoop net_loop2(packet_factory, buffer_factory, arena);
    CHECK(net_loop2.is_valid());

    UNSIGNED_LONGS_EQUAL(0, net_loop2.num_ports());

    CHECK(!add_udp_sender(net_loop2, tx_config));
    CHECK(!add_udp_receiver(net_loop2, rx_config, queue));

    UNSIGNED_LONGS_EQUAL(2, net_loop1.num_ports());
    UNSIGNED_LONGS_EQUAL(0, net_loop2.num_ports());
}

TEST(udp_ports, add_broadcast_sender) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    UdpConfig tx_config = make_udp_config("127.0.0.1", 0);
    NetworkLoop::PortHandle tx_handle = add_udp_sender(net_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());
}

TEST(udp_ports, add_multicast_receiver) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    { // miface empty
        UdpConfig rx_config = make_udp_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "");

        CHECK(add_udp_receiver(net_loop, rx_config, queue));
        UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());
    }
    { // miface 0.0.0.0
        UdpConfig rx_config = make_udp_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        CHECK(add_udp_receiver(net_loop, rx_config, queue));
        UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());
    }
}

TEST(udp_ports, add_multicast_receiver_error) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    { // non-multicast address
        UdpConfig rx_config = make_udp_config("127.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        CHECK(!add_udp_receiver(net_loop, rx_config, queue));
        UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
    }
    { // ipv6 miface for ipv4 addr
        UdpConfig rx_config = make_udp_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "::");

        CHECK(!add_udp_receiver(net_loop, rx_config, queue));
        UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
    }
    { // ipv4 miface for ipv6 addr
        UdpConfig rx_config = make_udp_config("::1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        CHECK(!add_udp_receiver(net_loop, rx_config, queue));
        UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
    }
}

TEST(udp_ports, add_bidirectional) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UdpConfig config = make_udp_config("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle handle = add_udp_sender_receiver(net_loop, config, queue);
    CHECK(handle);
    CHECK(config.bind_address.port() != 0);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());
}

TEST(udp_ports, add_remove) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);
    UdpConfig rx_config = make_udp_config("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle tx_handle = add_udp_sender(net_loop, tx_config);
    CHECK(tx_handle);

    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::PortHandle rx_handle = add_udp_receiver(net_loop, rx_config, queue);
    CHECK(rx_handle);

    UNSIGNED_LONGS_EQUAL(2, net_loop.num_ports());

    remove_port(net_loop, tx_handle);
    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, rx_handle);
    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(udp_ports, add_remove_add) {
    NetworkLoop net_loop(packet_factory, buffer_factory, arena);
    CHECK(net_loop.is_valid());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);

    NetworkLoop::PortHandle tx_handle = add_udp_sender(net_loop, tx_config);
    CHECK(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, tx_handle);
    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    tx_handle = add_udp_sender(net_loop, tx_config);
    CHECK(tx_handle);
    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());
}

} // namespace netio
} // namespace roc
