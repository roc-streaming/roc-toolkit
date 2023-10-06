/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"

namespace roc {
namespace packet {

namespace {

core::HeapArena arena;
PacketFactory packet_factory(arena);

PacketPtr new_packet(seqnum_t sn) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP);
    packet->rtp()->seqnum = sn;

    return packet;
}

} // namespace

TEST_GROUP(interleaver) {};

// Fill Interleaver with multiple of its internal memory size.
TEST(interleaver, read_write) {
    Queue queue;
    Interleaver intrlvr(queue, arena, 10);

    CHECK(intrlvr.is_valid());

    const size_t num_packets = intrlvr.block_size() * 5;

    // Packets to push to Interleaver.
    core::Array<PacketPtr> packets(arena);
    CHECK(packets.resize(num_packets));

    // Checks for received packets.
    core::Array<bool> packets_ctr(arena);
    CHECK(packets_ctr.resize(num_packets));

    for (size_t i = 0; i < num_packets; i++) {
        packets[i] = new_packet(seqnum_t(i));
        packets_ctr[i] = false;
    }

    // No packets in interleaver on start.
    LONGS_EQUAL(0, queue.size());

    // Push every packet to interleaver.
    for (size_t i = 0; i < num_packets; i++) {
        intrlvr.write(packets[i]);
    }

    // Interleaver must put all packets to its writer because we put pricesly
    // integer number of its block_size.
    LONGS_EQUAL(num_packets, queue.size());

    // Check that packets have different seqnums.
    for (size_t i = 0; i < num_packets; i++) {
        PacketPtr p;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(p));
        CHECK(p);
        CHECK(p->rtp()->seqnum < num_packets);
        CHECK(!packets_ctr[p->rtp()->seqnum]);
        packets_ctr[p->rtp()->seqnum] = true;
    }

    // Nothing left in queue.
    LONGS_EQUAL(0, queue.size());
    intrlvr.flush();

    // Nothing left in interleaver.
    LONGS_EQUAL(0, queue.size());

    // Did we receive all packets that we've sent.
    for (size_t i = 0; i < num_packets; i++) {
        CHECK(packets_ctr[i]);
    }
}

TEST(interleaver, flush) {
    Queue queue;
    Interleaver intrlvr(queue, arena, 10);

    CHECK(intrlvr.is_valid());

    const size_t num_packets = intrlvr.block_size() * 5;

    for (size_t n = 0; n < num_packets; n++) {
        PacketPtr wp = new_packet(seqnum_t(n));

        intrlvr.write(wp);
        intrlvr.flush();
        LONGS_EQUAL(1, queue.size());

        PacketPtr rp;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rp));
        CHECK(wp == rp);
        LONGS_EQUAL(0, queue.size());
    }
}

} // namespace packet
} // namespace roc
