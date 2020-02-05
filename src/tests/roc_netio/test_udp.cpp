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

enum { NumIterations = 20, NumPackets = 10, BufferSize = 125 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, BufferSize, true);
packet::PacketPool packet_pool(allocator, true);

UdpSenderConfig make_sender_config() {
    UdpSenderConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
    return config;
}

UdpReceiverConfig make_receiver_config() {
    UdpReceiverConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
    return config;
}

} // namespace

TEST_GROUP(udp) {
    core::Slice<uint8_t> new_buffer(int value) {
        core::Slice<uint8_t> buf = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        CHECK(buf);
        buf.resize(BufferSize);
        for (int n = 0; n < BufferSize; n++) {
            buf.data()[n] = uint8_t((value + n) & 0xff);
        }
        return buf;
    }

    packet::PacketPtr new_packet(const UdpSenderConfig& tx_config,
                                 const UdpReceiverConfig& rx_config,
                                 int value) {
        packet::PacketPtr pp = new (packet_pool) packet::Packet(packet_pool);
        CHECK(pp);

        pp->add_flags(packet::Packet::FlagUDP);

        pp->udp()->src_addr = tx_config.bind_address;
        pp->udp()->dst_addr = rx_config.bind_address;

        pp->set_data(new_buffer(value));

        return pp;
    }

    void check_packet(const packet::PacketPtr& pp,
                      const UdpSenderConfig& tx_config,
                      const UdpReceiverConfig& rx_config,
                      int value) {
        CHECK(pp);

        CHECK(pp->udp());
        CHECK(pp->data());

        CHECK(pp->udp()->src_addr == tx_config.bind_address);
        CHECK(pp->udp()->dst_addr == rx_config.bind_address);

        core::Slice<uint8_t> expected = new_buffer(value);

        UNSIGNED_LONGS_EQUAL(expected.size(), pp->data().size());
        CHECK(memcmp(pp->data().data(), expected.data(), expected.size()) == 0);
    }
};

TEST(udp, one_sender_one_receiver_single_thread) {
    packet::ConcurrentQueue rx_queue;

    UdpSenderConfig tx_config = make_sender_config();
    UdpReceiverConfig rx_config = make_receiver_config();

    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    packet::IWriter* tx_writer = NULL;
    CHECK(event_loop.add_udp_sender(tx_config, &tx_writer));
    CHECK(tx_writer);

    CHECK(event_loop.add_udp_receiver(rx_config, rx_queue));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            tx_writer->write(new_packet(tx_config, rx_config, p));
        }
        for (int p = 0; p < NumPackets; p++) {
            check_packet(rx_queue.read(), tx_config, rx_config, p);
        }
    }
}

TEST(udp, one_sender_one_receiver_separate_threads) {
    packet::ConcurrentQueue rx_queue;

    UdpSenderConfig tx_config = make_sender_config();
    UdpReceiverConfig rx_config = make_receiver_config();

    EventLoop tx_loop(packet_pool, buffer_pool, allocator);
    CHECK(tx_loop.valid());

    packet::IWriter* tx_writer = NULL;
    CHECK(tx_loop.add_udp_sender(tx_config, &tx_writer));
    CHECK(tx_writer);

    EventLoop rx_loop(packet_pool, buffer_pool, allocator);
    CHECK(rx_loop.valid());
    CHECK(rx_loop.add_udp_receiver(rx_config, rx_queue));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            tx_writer->write(new_packet(tx_config, rx_config, p));
        }
        for (int p = 0; p < NumPackets; p++) {
            check_packet(rx_queue.read(), tx_config, rx_config, p);
        }
    }
}

TEST(udp, one_sender_multiple_receivers) {
    packet::ConcurrentQueue rx_queue1;
    packet::ConcurrentQueue rx_queue2;
    packet::ConcurrentQueue rx_queue3;

    UdpSenderConfig tx_config = make_sender_config();

    UdpReceiverConfig rx_config1 = make_receiver_config();
    UdpReceiverConfig rx_config2 = make_receiver_config();
    UdpReceiverConfig rx_config3 = make_receiver_config();

    EventLoop tx_loop(packet_pool, buffer_pool, allocator);
    CHECK(tx_loop.valid());

    packet::IWriter* tx_writer = NULL;
    CHECK(tx_loop.add_udp_sender(tx_config, &tx_writer));
    CHECK(tx_writer);

    EventLoop rx1_loop(packet_pool, buffer_pool, allocator);
    CHECK(rx1_loop.valid());
    CHECK(rx1_loop.add_udp_receiver(rx_config1, rx_queue1));

    EventLoop rx23_loop(packet_pool, buffer_pool, allocator);
    CHECK(rx23_loop.valid());
    CHECK(rx23_loop.add_udp_receiver(rx_config2, rx_queue2));
    CHECK(rx23_loop.add_udp_receiver(rx_config3, rx_queue3));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            tx_writer->write(new_packet(tx_config, rx_config1, p * 10));
            tx_writer->write(new_packet(tx_config, rx_config2, p * 20));
            tx_writer->write(new_packet(tx_config, rx_config3, p * 30));
        }
        for (int p = 0; p < NumPackets; p++) {
            check_packet(rx_queue1.read(), tx_config, rx_config1, p * 10);
            check_packet(rx_queue2.read(), tx_config, rx_config2, p * 20);
            check_packet(rx_queue3.read(), tx_config, rx_config3, p * 30);
        }
    }
}

TEST(udp, multiple_senders_one_receiver) {
    packet::ConcurrentQueue rx_queue;

    UdpSenderConfig tx_config1 = make_sender_config();
    UdpSenderConfig tx_config2 = make_sender_config();
    UdpSenderConfig tx_config3 = make_sender_config();

    UdpReceiverConfig rx_config = make_receiver_config();

    EventLoop tx1_loop(packet_pool, buffer_pool, allocator);
    CHECK(tx1_loop.valid());

    packet::IWriter* tx_writer1 = NULL;
    CHECK(tx1_loop.add_udp_sender(tx_config1, &tx_writer1));
    CHECK(tx_writer1);

    EventLoop tx23_loop(packet_pool, buffer_pool, allocator);
    CHECK(tx23_loop.valid());

    packet::IWriter* tx_writer2 = NULL;
    CHECK(tx23_loop.add_udp_sender(tx_config2, &tx_writer2));
    CHECK(tx_writer2);

    packet::IWriter* tx_writer3 = NULL;
    CHECK(tx23_loop.add_udp_sender(tx_config3, &tx_writer3));
    CHECK(tx_writer3);

    EventLoop rx_loop(packet_pool, buffer_pool, allocator);
    CHECK(rx_loop.valid());
    CHECK(rx_loop.add_udp_receiver(rx_config, rx_queue));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            tx_writer1->write(new_packet(tx_config1, rx_config, p * 10));
        }
        for (int p = 0; p < NumPackets; p++) {
            check_packet(rx_queue.read(), tx_config1, rx_config, p * 10);
        }
        for (int p = 0; p < NumPackets; p++) {
            tx_writer2->write(new_packet(tx_config2, rx_config, p * 20));
        }
        for (int p = 0; p < NumPackets; p++) {
            check_packet(rx_queue.read(), tx_config2, rx_config, p * 20);
        }
        for (int p = 0; p < NumPackets; p++) {
            tx_writer3->write(new_packet(tx_config3, rx_config, p * 30));
        }
        for (int p = 0; p < NumPackets; p++) {
            check_packet(rx_queue.read(), tx_config3, rx_config, p * 30);
        }
    }
}

} // namespace netio
} // namespace roc
