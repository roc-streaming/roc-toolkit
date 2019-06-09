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
    CHECK(addr.set_ipv4(ip, port));
    return addr;
}

} // namespace

TEST_GROUP(transceiver) {};

TEST(transceiver, noop) {
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

TEST(transceiver, start_stop) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    CHECK(trx.start());
    trx.stop();
    trx.join();
}

TEST(transceiver, stop_start) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    trx.stop();
    CHECK(!trx.start());
    trx.join();
}

TEST(transceiver, start_start) {
    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    CHECK(trx.start());
    CHECK(!trx.start());
    trx.stop();
    trx.join();
}

TEST(transceiver, add_start_stop) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx_addr = make_address("0.0.0.0", 0);
    packet::Address rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    CHECK(trx.start());

    trx.stop();
    trx.join();

    trx.remove_port(tx_addr);
    trx.remove_port(rx_addr);
}

TEST(transceiver, start_add_stop) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    CHECK(trx.start());

    packet::Address tx_addr = make_address("0.0.0.0", 0);
    packet::Address rx_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    trx.stop();
    trx.join();

    trx.remove_port(tx_addr);
    trx.remove_port(rx_addr);
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

TEST(transceiver, start_add_remove_stop) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    CHECK(trx.start());

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

    trx.stop();
    trx.join();
}

TEST(transceiver, add_start_stop_remove) {
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

    CHECK(trx.start());

    trx.stop();
    trx.join();

    trx.remove_port(tx_addr);
    UNSIGNED_LONGS_EQUAL(1, trx.num_ports());

    trx.remove_port(rx_addr);
    UNSIGNED_LONGS_EQUAL(0, trx.num_ports());
}

TEST(transceiver, add_no_remove) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx1_addr = make_address("0.0.0.0", 0);
    packet::Address tx2_addr = make_address("0.0.0.0", 0);

    packet::Address rx1_addr = make_address("0.0.0.0", 0);
    packet::Address rx2_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx1_addr));
    CHECK(trx.add_udp_sender(tx2_addr));

    CHECK(trx.add_udp_receiver(rx1_addr, queue));
    CHECK(trx.add_udp_receiver(rx2_addr, queue));
}

TEST(transceiver, add_start_stop_no_remove) {
    packet::ConcurrentQueue queue;

    Transceiver trx(packet_pool, buffer_pool, allocator);

    CHECK(trx.valid());

    packet::Address tx1_addr = make_address("0.0.0.0", 0);
    packet::Address tx2_addr = make_address("0.0.0.0", 0);

    packet::Address rx1_addr = make_address("0.0.0.0", 0);
    packet::Address rx2_addr = make_address("0.0.0.0", 0);

    CHECK(trx.add_udp_sender(tx1_addr));
    CHECK(trx.add_udp_sender(tx2_addr));

    CHECK(trx.add_udp_receiver(rx1_addr, queue));
    CHECK(trx.add_udp_receiver(rx2_addr, queue));

    CHECK(trx.start());

    trx.stop();
    trx.join();
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
