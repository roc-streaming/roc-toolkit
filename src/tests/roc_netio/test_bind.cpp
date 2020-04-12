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
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port)
          || config.bind_address.set_host_port(address::Family_IPv6, ip, port));
    return config;
}

UdpReceiverConfig make_receiver_config(const char* ip, int port) {
    UdpReceiverConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port)
          || config.bind_address.set_host_port(address::Family_IPv6, ip, port));
    return config;
}

EventLoop::PortHandle add_udp_receiver(EventLoop& event_loop,
                                       UdpReceiverConfig& config,
                                       packet::IWriter& writer) {
    EventLoop::Tasks::AddUdpReceiverPort task(config, writer);
    CHECK(!task.success());
    if (!event_loop.enqueue_and_wait(task)) {
        CHECK(!task.success());
        return NULL;
    }
    CHECK(task.success());
    return task.get_handle();
}

EventLoop::PortHandle add_udp_sender(EventLoop& event_loop, UdpSenderConfig& config) {
    EventLoop::Tasks::AddUdpSenderPort task(config);
    CHECK(!task.success());
    if (!event_loop.enqueue_and_wait(task)) {
        CHECK(!task.success());
        return NULL;
    }
    CHECK(task.success());
    return task.get_handle();
}

void remove_port(EventLoop& event_loop, EventLoop::PortHandle handle) {
    EventLoop::Tasks::RemovePort task(handle);
    CHECK(!task.success());
    CHECK(event_loop.enqueue_and_wait(task));
    CHECK(task.success());
}

} // namespace

TEST_GROUP(bind) {};

TEST(bind, any) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpSenderConfig tx_config = make_sender_config("0.0.0.0", 0);
    UdpReceiverConfig rx_config = make_receiver_config("0.0.0.0", 0);

    EventLoop::PortHandle tx_handle = add_udp_sender(event_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    EventLoop::PortHandle rx_handle = add_udp_receiver(event_loop, rx_config, queue);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    remove_port(event_loop, tx_handle);
    remove_port(event_loop, rx_handle);
}

TEST(bind, localhost) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpSenderConfig tx_config = make_sender_config("127.0.0.1", 0);
    UdpReceiverConfig rx_config = make_receiver_config("127.0.0.1", 0);

    EventLoop::PortHandle tx_handle = add_udp_sender(event_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    EventLoop::PortHandle rx_handle = add_udp_receiver(event_loop, rx_config, queue);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    remove_port(event_loop, tx_handle);
    remove_port(event_loop, rx_handle);
}

TEST(bind, addrinuse) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop1(packet_pool, buffer_pool, allocator);
    CHECK(event_loop1.valid());

    UdpSenderConfig tx_config = make_sender_config("127.0.0.1", 0);
    UdpReceiverConfig rx_config = make_receiver_config("127.0.0.1", 0);

    EventLoop::PortHandle tx_handle = add_udp_sender(event_loop1, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);

    EventLoop::PortHandle rx_handle = add_udp_receiver(event_loop1, rx_config, queue);
    CHECK(rx_handle);
    CHECK(rx_config.bind_address.port() != 0);

    EventLoop event_loop2(packet_pool, buffer_pool, allocator);
    CHECK(event_loop2.valid());

    CHECK(!add_udp_sender(event_loop2, tx_config));
    CHECK(!add_udp_receiver(event_loop2, rx_config, queue));
}

TEST(bind, broadcast) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpSenderConfig tx_config = make_sender_config("127.0.0.1", 0);
    tx_config.broadcast_enabled = true;

    EventLoop::PortHandle tx_handle = add_udp_sender(event_loop, tx_config);
    CHECK(tx_handle);
    CHECK(tx_config.bind_address.port() != 0);
}

TEST(bind, multicast) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    { // miface empty
        UdpReceiverConfig rx_config = make_receiver_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "");

        CHECK(add_udp_receiver(event_loop, rx_config, queue));
    }
    { // miface 0.0.0.0
        UdpReceiverConfig rx_config = make_receiver_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        CHECK(add_udp_receiver(event_loop, rx_config, queue));
    }
}

TEST(bind, multicast_error) {
    packet::ConcurrentQueue queue;

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    { // non-multicast address
        UdpReceiverConfig rx_config = make_receiver_config("127.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        CHECK(!add_udp_receiver(event_loop, rx_config, queue));
    }
    { // ipv6 miface for ipv4 addr
        UdpReceiverConfig rx_config = make_receiver_config("224.0.0.1", 0);
        strcpy(rx_config.multicast_interface, "::");

        CHECK(!add_udp_receiver(event_loop, rx_config, queue));
    }
    { // ipv4 miface for ipv6 addr
        UdpReceiverConfig rx_config = make_receiver_config("::1", 0);
        strcpy(rx_config.multicast_interface, "0.0.0.0");

        CHECK(!add_udp_receiver(event_loop, rx_config, queue));
    }
}

} // namespace netio
} // namespace roc
