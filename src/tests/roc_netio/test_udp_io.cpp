/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_core/time.h"
#include "roc_netio/network_loop.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace netio {

namespace {

enum { NumIterations = 10, NumPackets = 7, BufferSize = 125 };

core::HeapArena arena;

core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);
core::SlabPool<core::Buffer>
    buffer_pool("buffer_pool", arena, sizeof(core::Buffer) + BufferSize);

packet::PacketFactory packet_factory(packet_pool, buffer_pool);

void short_delay() {
    core::sleep_for(core::ClockMonotonic, core::Microsecond * 500);
}

UdpConfig make_udp_config() {
    UdpConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
    return config;
}

NetworkLoop::PortHandle add_udp_sender(NetworkLoop& net_loop,
                                       UdpConfig& config,
                                       packet::IWriter** outbound_writer) {
    NetworkLoop::Tasks::AddUdpPort add_task(config);
    CHECK(!add_task.success());
    CHECK(net_loop.schedule_and_wait(add_task));
    CHECK(add_task.success());

    NetworkLoop::Tasks::StartUdpSend send_task(add_task.get_handle());
    CHECK(!send_task.success());
    CHECK(net_loop.schedule_and_wait(send_task));
    CHECK(send_task.success());
    *outbound_writer = &send_task.get_outbound_writer();

    return add_task.get_handle();
}

NetworkLoop::PortHandle add_udp_receiver(NetworkLoop& net_loop,
                                         UdpConfig& config,
                                         packet::IWriter& inbound_writer) {
    NetworkLoop::Tasks::AddUdpPort add_task(config);
    CHECK(!add_task.success());
    CHECK(net_loop.schedule_and_wait(add_task));
    CHECK(add_task.success());

    NetworkLoop::Tasks::StartUdpRecv recv_task(add_task.get_handle(), inbound_writer);
    CHECK(!recv_task.success());
    CHECK(net_loop.schedule_and_wait(recv_task));
    CHECK(recv_task.success());

    return add_task.get_handle();
}

NetworkLoop::PortHandle add_udp_sender_receiver(NetworkLoop& net_loop,
                                                UdpConfig& config,
                                                packet::IWriter& inbound_writer,
                                                packet::IWriter** outbound_writer) {
    NetworkLoop::Tasks::AddUdpPort add_task(config);
    CHECK(!add_task.success());
    CHECK(net_loop.schedule_and_wait(add_task));
    CHECK(add_task.success());

    NetworkLoop::Tasks::StartUdpRecv recv_task(add_task.get_handle(), inbound_writer);
    CHECK(!recv_task.success());
    CHECK(net_loop.schedule_and_wait(recv_task));
    CHECK(recv_task.success());

    NetworkLoop::Tasks::StartUdpSend send_task(add_task.get_handle());
    CHECK(!send_task.success());
    CHECK(net_loop.schedule_and_wait(send_task));
    CHECK(send_task.success());
    *outbound_writer = &send_task.get_outbound_writer();

    return add_task.get_handle();
}

core::Slice<uint8_t> new_buffer(int value) {
    core::Slice<uint8_t> buf = packet_factory.new_packet_buffer();
    CHECK(buf);
    buf.reslice(0, BufferSize);
    for (int n = 0; n < BufferSize; n++) {
        buf.data()[n] = uint8_t((value + n) & 0xff);
    }
    return buf;
}

packet::PacketPtr
new_packet(const UdpConfig& tx_config, const UdpConfig& rx_config, int value) {
    packet::PacketPtr pp = packet_factory.new_packet();
    CHECK(pp);

    pp->add_flags(packet::Packet::FlagUDP);

    pp->udp()->src_addr = tx_config.bind_address;
    pp->udp()->dst_addr = rx_config.bind_address;

    pp->set_buffer(new_buffer(value));

    return pp;
}

void dump_packet(const packet::PacketPtr& pp,
                 const address::SocketAddr& expected_src_addr,
                 const address::SocketAddr& expected_dst_addr,
                 const core::Slice<uint8_t>& expected_buf,
                 int value,
                 int iteration) {
    core::sleep_for(core::ClockMonotonic, core::Second);

    fprintf(stderr, "iteration:  %d  value:  %d\n", iteration, value);
    fprintf(stderr, "expected src_addr:  %s\n",
            address::socket_addr_to_str(expected_src_addr).c_str());
    fprintf(stderr, "received src_addr:  %s\n",
            address::socket_addr_to_str(pp->udp()->src_addr).c_str());
    fprintf(stderr, "expected dst_addr:  %s\n",
            address::socket_addr_to_str(expected_dst_addr).c_str());
    fprintf(stderr, "received dst_addr:  %s\n",
            address::socket_addr_to_str(pp->udp()->dst_addr).c_str());
    fprintf(stderr, "expected buffer:\n");
    expected_buf.print();
    fprintf(stderr, "received buffer:\n");
    pp->buffer().print();
}

void check_packet(const packet::PacketPtr& pp,
                  const UdpConfig& tx_config,
                  const UdpConfig& rx_config,
                  int value,
                  int iteration) {
    CHECK(pp);

    CHECK(pp->udp());
    CHECK(pp->buffer());

    address::SocketAddr expected_src_addr = tx_config.bind_address;
    address::SocketAddr expected_dst_addr = rx_config.bind_address;

    core::Slice<uint8_t> expected_buf = new_buffer(value);

    if (pp->udp()->src_addr != expected_src_addr) {
        dump_packet(pp, expected_src_addr, expected_dst_addr, expected_buf, iteration,
                    value);
        FAIL("receiver src_addr does not match expected");
    }

    if (pp->udp()->dst_addr != expected_dst_addr) {
        dump_packet(pp, expected_src_addr, expected_dst_addr, expected_buf, iteration,
                    value);
        FAIL("receiver dst_addr does not match expected");
    }

    if (pp->buffer().size() != expected_buf.size()
        || memcmp(pp->buffer().data(), expected_buf.data(), expected_buf.size()) != 0) {
        dump_packet(pp, expected_src_addr, expected_dst_addr, expected_buf, iteration,
                    value);
        FAIL("received buffer does not match expected");
    }
}

void write_packet(packet::IWriter& writer, const packet::PacketPtr& packet) {
    CHECK(packet);
    LONGS_EQUAL(status::StatusOK, writer.write(packet));
}

packet::PacketPtr read_packet(packet::IReader& reader) {
    packet::PacketPtr packet;
    LONGS_EQUAL(status::StatusOK, reader.read(packet, packet::ModeFetch));
    CHECK(packet);
    return packet;
}

} // namespace

TEST_GROUP(udp_io) {};

TEST(udp_io, one_sender_one_receiver_single_thread_non_blocking_disabled) {
    packet::ConcurrentQueue rx_queue(packet::ConcurrentQueue::Blocking);

    UdpConfig tx_config = make_udp_config();
    UdpConfig rx_config = make_udp_config();

    tx_config.enable_non_blocking = false;

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    packet::IWriter* tx_writer = NULL;
    CHECK(add_udp_sender(net_loop, tx_config, &tx_writer));
    CHECK(tx_writer);

    CHECK(add_udp_receiver(net_loop, rx_config, rx_queue));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*tx_writer, new_packet(tx_config, rx_config, p));
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(rx_queue);
            check_packet(pp, tx_config, rx_config, p, i);
        }
    }
}

TEST(udp_io, one_sender_one_receiver_single_loop) {
    packet::ConcurrentQueue rx_queue(packet::ConcurrentQueue::Blocking);

    UdpConfig tx_config = make_udp_config();
    UdpConfig rx_config = make_udp_config();

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    packet::IWriter* tx_writer = NULL;
    CHECK(add_udp_sender(net_loop, tx_config, &tx_writer));
    CHECK(tx_writer);

    CHECK(add_udp_receiver(net_loop, rx_config, rx_queue));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*tx_writer, new_packet(tx_config, rx_config, p));
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(rx_queue);
            check_packet(pp, tx_config, rx_config, p, i);
        }
    }
}

TEST(udp_io, one_sender_one_receiver_separate_loops) {
    packet::ConcurrentQueue rx_queue(packet::ConcurrentQueue::Blocking);

    UdpConfig tx_config = make_udp_config();
    UdpConfig rx_config = make_udp_config();

    NetworkLoop tx_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, tx_loop.init_status());

    packet::IWriter* tx_writer = NULL;
    CHECK(add_udp_sender(tx_loop, tx_config, &tx_writer));
    CHECK(tx_writer);

    NetworkLoop rx_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, rx_loop.init_status());
    CHECK(add_udp_receiver(rx_loop, rx_config, rx_queue));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*tx_writer, new_packet(tx_config, rx_config, p));
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(rx_queue);
            check_packet(pp, tx_config, rx_config, p, i);
        }
    }
}

TEST(udp_io, one_sender_many_receivers) {
    packet::ConcurrentQueue rx_queue1(packet::ConcurrentQueue::Blocking);
    packet::ConcurrentQueue rx_queue2(packet::ConcurrentQueue::Blocking);
    packet::ConcurrentQueue rx_queue3(packet::ConcurrentQueue::Blocking);

    UdpConfig tx_config = make_udp_config();

    UdpConfig rx_config1 = make_udp_config();
    UdpConfig rx_config2 = make_udp_config();
    UdpConfig rx_config3 = make_udp_config();

    NetworkLoop tx_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, tx_loop.init_status());

    packet::IWriter* tx_writer = NULL;
    CHECK(add_udp_sender(tx_loop, tx_config, &tx_writer));
    CHECK(tx_writer);

    NetworkLoop rx1_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, rx1_loop.init_status());
    CHECK(add_udp_receiver(rx1_loop, rx_config1, rx_queue1));

    NetworkLoop rx23_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, rx23_loop.init_status());
    CHECK(add_udp_receiver(rx23_loop, rx_config2, rx_queue2));
    CHECK(add_udp_receiver(rx23_loop, rx_config3, rx_queue3));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*tx_writer, new_packet(tx_config, rx_config1, p * 10));
            write_packet(*tx_writer, new_packet(tx_config, rx_config2, p * 20));
            write_packet(*tx_writer, new_packet(tx_config, rx_config3, p * 30));
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp1 = read_packet(rx_queue1);
            check_packet(pp1, tx_config, rx_config1, p * 10, i);

            packet::PacketPtr pp2 = read_packet(rx_queue2);
            check_packet(pp2, tx_config, rx_config2, p * 20, i);

            packet::PacketPtr pp3 = read_packet(rx_queue3);
            check_packet(pp3, tx_config, rx_config3, p * 30, i);
        }
    }
}

TEST(udp_io, many_senders_one_receiver) {
    packet::ConcurrentQueue rx_queue(packet::ConcurrentQueue::Blocking);

    UdpConfig tx_config1 = make_udp_config();
    UdpConfig tx_config2 = make_udp_config();
    UdpConfig tx_config3 = make_udp_config();

    UdpConfig rx_config = make_udp_config();

    NetworkLoop tx1_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, tx1_loop.init_status());

    packet::IWriter* tx_writer1 = NULL;
    CHECK(add_udp_sender(tx1_loop, tx_config1, &tx_writer1));
    CHECK(tx_writer1);

    NetworkLoop tx23_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, tx23_loop.init_status());

    packet::IWriter* tx_writer2 = NULL;
    CHECK(add_udp_sender(tx23_loop, tx_config2, &tx_writer2));
    CHECK(tx_writer2);

    packet::IWriter* tx_writer3 = NULL;
    CHECK(add_udp_sender(tx23_loop, tx_config3, &tx_writer3));
    CHECK(tx_writer3);

    NetworkLoop rx_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, rx_loop.init_status());
    CHECK(add_udp_receiver(rx_loop, rx_config, rx_queue));

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*tx_writer1, new_packet(tx_config1, rx_config, p * 10));
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(rx_queue);
            check_packet(pp, tx_config1, rx_config, p * 10, i);
        }

        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*tx_writer2, new_packet(tx_config2, rx_config, p * 20));
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(rx_queue);
            check_packet(pp, tx_config2, rx_config, p * 20, i);
        }

        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*tx_writer3, new_packet(tx_config3, rx_config, p * 30));
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(rx_queue);
            check_packet(pp, tx_config3, rx_config, p * 30, i);
        }
    }
}

TEST(udp_io, bidirectional_ports_one_loop) {
    packet::ConcurrentQueue peer1_rx_queue(packet::ConcurrentQueue::Blocking);
    packet::ConcurrentQueue peer2_rx_queue(packet::ConcurrentQueue::Blocking);

    UdpConfig peer1_config = make_udp_config();
    UdpConfig peer2_config = make_udp_config();

    peer1_config.enable_non_blocking = false;
    peer2_config.enable_non_blocking = false;

    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    packet::IWriter* peer1_tx_writer = NULL;
    CHECK(add_udp_sender_receiver(net_loop, peer1_config, peer1_rx_queue,
                                  &peer1_tx_writer));
    CHECK(peer1_tx_writer);

    packet::IWriter* peer2_tx_writer = NULL;
    CHECK(add_udp_sender_receiver(net_loop, peer2_config, peer2_rx_queue,
                                  &peer2_tx_writer));
    CHECK(peer2_tx_writer);

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*peer1_tx_writer, new_packet(peer1_config, peer2_config, p));
        }
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*peer2_tx_writer, new_packet(peer2_config, peer1_config, p));
        }

        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(peer2_rx_queue);
            check_packet(pp, peer1_config, peer2_config, p, i);
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(peer1_rx_queue);
            check_packet(pp, peer2_config, peer1_config, p, i);
        }
    }
}

TEST(udp_io, bidirectional_ports_separate_loops) {
    packet::ConcurrentQueue peer1_rx_queue(packet::ConcurrentQueue::Blocking);
    packet::ConcurrentQueue peer2_rx_queue(packet::ConcurrentQueue::Blocking);

    UdpConfig peer1_config = make_udp_config();
    UdpConfig peer2_config = make_udp_config();

    peer1_config.enable_non_blocking = false;
    peer2_config.enable_non_blocking = false;

    NetworkLoop peer1_net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, peer1_net_loop.init_status());

    NetworkLoop peer2_net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, peer2_net_loop.init_status());

    packet::IWriter* peer1_tx_writer = NULL;
    CHECK(add_udp_sender_receiver(peer1_net_loop, peer1_config, peer1_rx_queue,
                                  &peer1_tx_writer));
    CHECK(peer1_tx_writer);

    packet::IWriter* peer2_tx_writer = NULL;
    CHECK(add_udp_sender_receiver(peer2_net_loop, peer2_config, peer2_rx_queue,
                                  &peer2_tx_writer));
    CHECK(peer2_tx_writer);

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*peer1_tx_writer, new_packet(peer1_config, peer2_config, p));
        }
        for (int p = 0; p < NumPackets; p++) {
            short_delay();
            write_packet(*peer2_tx_writer, new_packet(peer2_config, peer1_config, p));
        }

        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(peer2_rx_queue);
            check_packet(pp, peer1_config, peer2_config, p, i);
        }
        for (int p = 0; p < NumPackets; p++) {
            packet::PacketPtr pp = read_packet(peer1_rx_queue);
            check_packet(pp, peer2_config, peer1_config, p, i);
        }
    }
}

} // namespace netio
} // namespace roc
