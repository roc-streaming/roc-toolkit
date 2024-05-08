/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_netio/network_loop.h"
#include "roc_packet/concurrent_queue.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapArena arena;
core::SlabPool<core::Buffer> buffer_pool("buffer_pool", arena, MaxBufSize);
core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);

UdpConfig make_udp_config(const char* ip, int port) {
    UdpConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port)
          || config.bind_address.set_host_port(address::Family_IPv6, ip, port));
    return config;
}

void remove_port(NetworkLoop& net_loop, NetworkLoop::PortHandle handle) {
    NetworkLoop::Tasks::RemovePort remove_task(handle);
    CHECK(!remove_task.success());
    CHECK(net_loop.schedule_and_wait(remove_task));
    CHECK(remove_task.success());
}

NetworkLoop::PortHandle add_port(NetworkLoop& net_loop, UdpConfig& config) {
    NetworkLoop::Tasks::AddUdpPort add_task(config);
    CHECK(!add_task.success());
    if (!net_loop.schedule_and_wait(add_task)) {
        CHECK(!add_task.success());
        return NULL;
    }
    CHECK(add_task.success());
    return add_task.get_handle();
}

bool start_send(NetworkLoop& net_loop, NetworkLoop::PortHandle port_handle) {
    NetworkLoop::Tasks::StartUdpSend send_task(port_handle);
    CHECK(!send_task.success());
    if (!net_loop.schedule_and_wait(send_task)) {
        CHECK(!send_task.success());
        return false;
    }
    CHECK(send_task.success());
    return true;
}

bool start_recv(NetworkLoop& net_loop,
                NetworkLoop::PortHandle port_handle,
                packet::IWriter& inbound_writer) {
    NetworkLoop::Tasks::StartUdpRecv recv_task(port_handle, inbound_writer);
    CHECK(!recv_task.success());
    if (!net_loop.schedule_and_wait(recv_task)) {
        CHECK(!recv_task.success());
        return false;
    }
    CHECK(recv_task.success());
    return true;
}

} // namespace

TEST_GROUP(udp_ports) {};

TEST(udp_ports, no_ports) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(udp_ports, add_remove) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);
    UdpConfig rx_config = make_udp_config("0.0.0.0", 0);

    LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);

    LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
    CHECK(rx_handle);

    LONGS_EQUAL(2, net_loop.num_ports());

    remove_port(net_loop, tx_handle);
    LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, rx_handle);
    LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(udp_ports, add_start_remove) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);
    UdpConfig rx_config = make_udp_config("0.0.0.0", 0);

    LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);

    LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
    CHECK(rx_handle);

    LONGS_EQUAL(2, net_loop.num_ports());

    CHECK(start_send(net_loop, tx_handle));
    CHECK(start_recv(net_loop, rx_handle, queue));

    LONGS_EQUAL(2, net_loop.num_ports());

    remove_port(net_loop, tx_handle);
    LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, rx_handle);
    LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(udp_ports, add_remove_add) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);

    NetworkLoop::PortHandle tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);
    LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, tx_handle);
    LONGS_EQUAL(0, net_loop.num_ports());

    tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);
    LONGS_EQUAL(1, net_loop.num_ports());
}

TEST(udp_ports, add_start_remove_add) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);

    NetworkLoop::PortHandle tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);
    LONGS_EQUAL(1, net_loop.num_ports());

    CHECK(start_send(net_loop, tx_handle));
    LONGS_EQUAL(1, net_loop.num_ports());

    remove_port(net_loop, tx_handle);
    LONGS_EQUAL(0, net_loop.num_ports());

    tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);
    LONGS_EQUAL(1, net_loop.num_ports());
}

TEST(udp_ports, anyaddr) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig tx_config = make_udp_config("0.0.0.0", 0);
    UdpConfig rx_config = make_udp_config("0.0.0.0", 0);

    LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    LONGS_EQUAL(2, net_loop.num_ports());

    CHECK(start_send(net_loop, tx_handle));
    CHECK(start_recv(net_loop, rx_handle, queue));

    LONGS_EQUAL(2, net_loop.num_ports());
}

TEST(udp_ports, localhost) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig tx_config = make_udp_config("127.0.0.1", 0);
    UdpConfig rx_config = make_udp_config("127.0.0.1", 0);

    LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    LONGS_EQUAL(2, net_loop.num_ports());

    CHECK(start_send(net_loop, tx_handle));
    CHECK(start_recv(net_loop, rx_handle, queue));

    LONGS_EQUAL(2, net_loop.num_ports());
}

TEST(udp_ports, addrinuse) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop1(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop1.init_status());

    UdpConfig tx_config = make_udp_config("127.0.0.1", 0);
    UdpConfig rx_config = make_udp_config("127.0.0.1", 0);

    LONGS_EQUAL(0, net_loop1.num_ports());

    NetworkLoop::PortHandle tx_handle = add_port(net_loop1, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    LONGS_EQUAL(1, net_loop1.num_ports());

    NetworkLoop::PortHandle rx_handle = add_port(net_loop1, rx_config);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    LONGS_EQUAL(2, net_loop1.num_ports());

    CHECK(start_send(net_loop1, tx_handle));
    CHECK(start_recv(net_loop1, rx_handle, queue));

    LONGS_EQUAL(2, net_loop1.num_ports());

    NetworkLoop net_loop2(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop2.init_status());

    LONGS_EQUAL(0, net_loop2.num_ports());

    CHECK(!add_port(net_loop2, tx_config));
    CHECK(!add_port(net_loop2, rx_config));

    LONGS_EQUAL(2, net_loop1.num_ports());
    LONGS_EQUAL(0, net_loop2.num_ports());
}

TEST(udp_ports, broadcast_sender) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    LONGS_EQUAL(0, net_loop.num_ports());

    UdpConfig tx_config = make_udp_config("127.0.0.1", 0);
    NetworkLoop::PortHandle tx_handle = add_port(net_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    CHECK(start_send(net_loop, tx_handle));

    LONGS_EQUAL(1, net_loop.num_ports());
}

TEST(udp_ports, multicast_receiver) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    LONGS_EQUAL(0, net_loop.num_ports());

    { // miface empty
        UdpConfig rx_config = make_udp_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "");

        NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
        CHECK(rx_handle);
        CHECK(start_recv(net_loop, rx_handle, queue));

        remove_port(net_loop, rx_handle);
    }
    { // miface 0.0.0.0
        UdpConfig rx_config = make_udp_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
        CHECK(rx_handle);
        CHECK(start_recv(net_loop, rx_handle, queue));

        remove_port(net_loop, rx_handle);
    }

    LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(udp_ports, multicast_receiver_error) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    LONGS_EQUAL(0, net_loop.num_ports());

    { // non-multicast address
        UdpConfig rx_config = make_udp_config("127.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
        CHECK(rx_handle);
        CHECK(!start_recv(net_loop, rx_handle, queue));

        remove_port(net_loop, rx_handle);
    }
    { // ipv6 miface for ipv4 addr
        UdpConfig rx_config = make_udp_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "::");

        NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
        CHECK(rx_handle);
        CHECK(!start_recv(net_loop, rx_handle, queue));

        remove_port(net_loop, rx_handle);
    }
    { // ipv4 miface for ipv6 addr
        UdpConfig rx_config = make_udp_config("::1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        NetworkLoop::PortHandle rx_handle = add_port(net_loop, rx_config);
        if (rx_handle) {
            CHECK(!start_recv(net_loop, rx_handle, queue));
            remove_port(net_loop, rx_handle);
        }
    }

    LONGS_EQUAL(0, net_loop.num_ports());
}

TEST(udp_ports, bidirectional) {
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig config = make_udp_config("0.0.0.0", 0);

    LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::PortHandle handle = add_port(net_loop, config);
    CHECK(handle);
    CHECK(config.bind_address.port() != 0);

    CHECK(start_send(net_loop, handle));
    CHECK(start_recv(net_loop, handle, queue));

    LONGS_EQUAL(1, net_loop.num_ports());
}

} // namespace netio
} // namespace roc
