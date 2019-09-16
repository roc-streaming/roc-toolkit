/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

packet::Address make_address(const char* ip, int port) {
    packet::Address addr;
    CHECK(addr.set_host_ipv4(ip, port));
    return addr;
}

} // namespace

TEST_GROUP(transceiver) {};

TEST(transceiver, init) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());
}

TEST(transceiver, bind_any) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx_addr = make_address("0.0.0.0", 0);
    packet::Address rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    trx.remove_port(tx_addr);
    trx.remove_port(rx_addr);
}

TEST(transceiver, bind_lo) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx_addr = make_address("127.0.0.1", 0);
    packet::Address rx_addr = make_address("127.0.0.1", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    trx.remove_port(tx_addr);
    trx.remove_port(rx_addr);
}

TEST(transceiver, bind_addrinuse) {
    packet::ConcurrentQueue queue;

    Transceiver trx1(packet_pool, buffer_pool, allocator);
    CHECK(trx1.valid());

    packet::Address tx_addr = make_address("127.0.0.1", 0);
    packet::Address rx_addr = make_address("127.0.0.1", 0);

    CHECK(trx1.add_udp_sender(tx_addr));
    CHECK(trx1.add_udp_receiver(rx_addr, queue));

    Transceiver trx2(packet_pool, buffer_pool, allocator);
    CHECK(trx2.valid());

    CHECK(!trx2.add_udp_sender(tx_addr));
    CHECK(!trx2.add_udp_receiver(rx_addr, queue));
}

TEST(transceiver, add) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx_addr = make_address("0.0.0.0", 0);
    packet::Address rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));
}

TEST(transceiver, add_remove) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx_addr = make_address("0.0.0.0", 0);
    packet::Address rx_addr = make_address("0.0.0.0", 0);

    UNSIGNED_LONGS_EQUAL(0, trx.num_ports());

    CHECK(trx.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(trx.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    trx.remove_port(tx_addr);
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    trx.remove_port(rx_addr);
    UNSIGNED_LONGS_EQUAL(0, trx.num_ports());
}

TEST(transceiver, add_remove_add) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    trx.remove_port(tx_addr);
    CHECK(trx.add_udp_sender(tx_addr));
}

TEST(transceiver, add_duplicate) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx_addr = make_address("0.0.0.0", 0);
    packet::Address rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(!trx.add_udp_sender(tx_addr));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(!trx.add_udp_receiver(tx_addr, queue));
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    CHECK(trx.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    CHECK(!trx.add_udp_sender(rx_addr));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    CHECK(!trx.add_udp_receiver(rx_addr, queue));
    UNSIGNED_LONGS_EQUAL(2, trx.num_ports());

    trx.remove_port(tx_addr);
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    trx.remove_port(rx_addr);
    UNSIGNED_LONGS_EQUAL(0, trx.num_ports());
}

} // namespace netio
} // namespace roc
