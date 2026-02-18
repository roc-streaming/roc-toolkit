/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace packet {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;
PacketFactory packet_factory(arena, MaxBufSize);

PacketPtr new_packet() {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP);

    return packet;
}

void expect_write(status::StatusCode expect_code, FifoQueue& queue, const PacketPtr& pp) {
    CHECK(pp);
    LONGS_EQUAL(expect_code, queue.write(pp));
}

PacketPtr
expect_read(status::StatusCode expect_code, IReader& queue, PacketReadMode mode) {
    PacketPtr pp;
    LONGS_EQUAL(expect_code, queue.read(pp, mode));
    if (expect_code == status::StatusOK) {
        CHECK(pp);
    } else {
        CHECK(!pp);
    }
    return pp;
}

} // namespace

TEST_GROUP(fifo_queue) {};

TEST(fifo_queue, empty) {
    FifoQueue queue;

    CHECK(!queue.head());
    CHECK(!queue.tail());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);

    LONGS_EQUAL(0, queue.size());
}

TEST(fifo_queue, two_packets) {
    FifoQueue queue;

    PacketPtr wp1 = new_packet();
    PacketPtr wp2 = new_packet();

    expect_write(status::StatusOK, queue, wp1);
    expect_write(status::StatusOK, queue, wp2);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == wp1);
    CHECK(queue.tail() == wp2);

    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.head() == wp2);
    CHECK(queue.tail() == wp2);

    PacketPtr rp2 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp2 == rp2);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.head());
    CHECK(!queue.tail());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);

    LONGS_EQUAL(0, queue.size());
}

TEST(fifo_queue, many_packets) {
    enum { NumPackets = 10 };

    FifoQueue queue;

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet();
    }

    for (ssize_t n = 0; n < NumPackets; n++) {
        expect_write(status::StatusOK, queue, packets[n]);
    }

    LONGS_EQUAL(NumPackets, queue.size());

    CHECK(queue.head() == packets[0]);
    CHECK(queue.tail() == packets[NumPackets - 1]);

    for (size_t n = 0; n < NumPackets; n++) {
        PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);
        CHECK(pp == packets[n]);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(fifo_queue, fetch_peek) {
    FifoQueue queue;

    {
        expect_read(status::StatusDrain, queue, ModePeek);
        expect_read(status::StatusDrain, queue, ModeFetch);
    }

    PacketPtr wp1 = new_packet();
    PacketPtr wp2 = new_packet();

    expect_write(status::StatusOK, queue, wp1);
    expect_write(status::StatusOK, queue, wp2);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == wp1);
    CHECK(queue.tail() == wp2);

    {
        PacketPtr rp = expect_read(status::StatusOK, queue, ModePeek);
        CHECK(wp1 == rp);
    }

    {
        PacketPtr rp = expect_read(status::StatusOK, queue, ModePeek);
        CHECK(wp1 == rp);
    }

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == wp1);
    CHECK(queue.tail() == wp2);

    {
        PacketPtr rp = expect_read(status::StatusOK, queue, ModeFetch);
        CHECK(wp1 == rp);
    }

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.head() == wp2);
    CHECK(queue.tail() == wp2);
}

} // namespace packet
} // namespace roc
