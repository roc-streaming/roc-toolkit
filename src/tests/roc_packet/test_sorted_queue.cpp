/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/sorted_queue.h"

namespace roc {
namespace packet {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;
PacketFactory packet_factory(arena, MaxBufSize);

PacketPtr new_packet(seqnum_t sn) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP);
    packet->rtp()->seqnum = sn;

    return packet;
}

void expect_write(status::StatusCode expect_code,
                  SortedQueue& queue,
                  const PacketPtr& pp) {
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

TEST_GROUP(sorted_queue) {};

TEST(sorted_queue, empty) {
    SortedQueue queue(0);

    CHECK(!queue.head());
    CHECK(!queue.tail());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, two_packets) {
    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(2);

    expect_write(status::StatusOK, queue, wp2);
    expect_write(status::StatusOK, queue, wp1);

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

TEST(sorted_queue, many_packets) {
    enum { NumPackets = 10 };

    SortedQueue queue(0);

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet(n);
    }

    for (ssize_t n = 0; n < NumPackets; n++) {
        expect_write(status::StatusOK, queue, packets[(n + NumPackets / 2) % NumPackets]);
    }

    LONGS_EQUAL(NumPackets, queue.size());

    CHECK(queue.head() == packets[0]);
    CHECK(queue.tail() == packets[NumPackets - 1]);

    for (size_t n = 0; n < NumPackets; n++) {
        CHECK(queue.head() == packets[n]);
        CHECK(queue.tail() == packets[NumPackets - 1]);

        PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);
        CHECK(pp == packets[n]);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, fetch_peek) {
    SortedQueue queue(0);

    {
        expect_read(status::StatusDrain, queue, ModePeek);
        expect_read(status::StatusDrain, queue, ModeFetch);
    }

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(2);

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

TEST(sorted_queue, out_of_order) {
    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(2);

    expect_write(status::StatusOK, queue, wp2);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.head() == wp2);
    CHECK(queue.tail() == wp2);

    PacketPtr rp2 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp2 == rp2);

    LONGS_EQUAL(0, queue.size());

    expect_write(status::StatusOK, queue, wp1);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.head() == wp1);
    CHECK(queue.tail() == wp1);

    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);

    CHECK(!queue.head());
    CHECK(!queue.tail());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);
}

TEST(sorted_queue, out_of_order_many_packets) {
    enum { NumPackets = 20 };

    SortedQueue queue(0);

    for (packet::seqnum_t n = 0; n < 7; ++n) {
        expect_write(status::StatusOK, queue, new_packet(n));
    }

    for (packet::seqnum_t n = 11; n < NumPackets; ++n) {
        expect_write(status::StatusOK, queue, new_packet(n));
    }

    for (packet::seqnum_t n = 0; n < 7; ++n) {
        packet::PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);

        CHECK(pp);
        LONGS_EQUAL(n, pp->rtp()->seqnum);
    }

    expect_write(status::StatusOK, queue, new_packet(9));
    expect_write(status::StatusOK, queue, new_packet(10));

    for (packet::seqnum_t n = 9; n < NumPackets; ++n) {
        packet::PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);

        LONGS_EQUAL(n, pp->rtp()->seqnum);

        if (n == 10) {
            expect_write(status::StatusOK, queue, new_packet(8));
            expect_write(status::StatusOK, queue, new_packet(7));

            pp = expect_read(status::StatusOK, queue, ModeFetch);
            LONGS_EQUAL(7, pp->rtp()->seqnum);

            pp = expect_read(status::StatusOK, queue, ModeFetch);
            LONGS_EQUAL(8, pp->rtp()->seqnum);
        }
    }
}

TEST(sorted_queue, one_duplicate) {
    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(1);

    expect_write(status::StatusOK, queue, wp1);
    expect_write(status::StatusOK, queue, wp2);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.head() == wp1);
    CHECK(queue.tail() == wp1);

    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.head());
    CHECK(!queue.tail());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);
}

TEST(sorted_queue, many_duplicates) {
    const size_t NumPackets = 10;

    SortedQueue queue(0);

    for (seqnum_t n = 0; n < NumPackets; n++) {
        expect_write(status::StatusOK, queue, new_packet(n));
    }

    LONGS_EQUAL(NumPackets, queue.size());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        expect_write(status::StatusOK, queue, new_packet(n));
    }

    LONGS_EQUAL(NumPackets, queue.size());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);
        LONGS_EQUAL(n, pp->rtp()->seqnum);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, max_size) {
    SortedQueue queue(2);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(2);
    PacketPtr wp3 = new_packet(3);

    expect_write(status::StatusOK, queue, wp1);
    expect_write(status::StatusOK, queue, wp2);
    expect_write(status::StatusOK, queue, wp3);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == wp1);
    CHECK(queue.tail() == wp2);

    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);

    LONGS_EQUAL(1, queue.size());

    expect_write(status::StatusOK, queue, wp3);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == wp2);
    CHECK(queue.tail() == wp3);
}

TEST(sorted_queue, overflow_ordered1) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(seqnum_t(sn - 10));
    PacketPtr wp2 = new_packet(sn);
    PacketPtr wp3 = new_packet(seqnum_t(sn + 10));

    expect_write(status::StatusOK, queue, wp1);
    expect_write(status::StatusOK, queue, wp2);
    expect_write(status::StatusOK, queue, wp3);

    LONGS_EQUAL(3, queue.size());

    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    PacketPtr rp2 = expect_read(status::StatusOK, queue, ModeFetch);
    PacketPtr rp3 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);
    CHECK(wp2 == rp2);
    CHECK(wp3 == rp3);

    LONGS_EQUAL(0, queue.size());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);
}

TEST(sorted_queue, overflow_ordered2) {
    const seqnum_t sn = seqnum_t(-1) >> 1;

    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(seqnum_t(sn - 10));
    PacketPtr wp2 = new_packet(sn);
    PacketPtr wp3 = new_packet(seqnum_t(sn + 10));

    expect_write(status::StatusOK, queue, wp1);
    expect_write(status::StatusOK, queue, wp2);
    expect_write(status::StatusOK, queue, wp3);

    LONGS_EQUAL(3, queue.size());

    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    PacketPtr rp2 = expect_read(status::StatusOK, queue, ModeFetch);
    PacketPtr rp3 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);
    CHECK(wp2 == rp2);
    CHECK(wp3 == rp3);

    LONGS_EQUAL(0, queue.size());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);
}

TEST(sorted_queue, overflow_sorting) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(seqnum_t(sn - 10));
    PacketPtr wp2 = new_packet(sn);
    PacketPtr wp3 = new_packet(seqnum_t(sn + 10));

    expect_write(status::StatusOK, queue, wp2);
    expect_write(status::StatusOK, queue, wp1);
    expect_write(status::StatusOK, queue, wp3);

    LONGS_EQUAL(3, queue.size());

    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    PacketPtr rp2 = expect_read(status::StatusOK, queue, ModeFetch);
    PacketPtr rp3 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);
    CHECK(wp2 == rp2);
    CHECK(wp3 == rp3);

    LONGS_EQUAL(0, queue.size());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);
}

TEST(sorted_queue, overflow_out_of_order) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(seqnum_t(sn - 10));
    PacketPtr wp2 = new_packet(sn);
    PacketPtr wp3 = new_packet(sn / 2);

    expect_write(status::StatusOK, queue, wp1);

    LONGS_EQUAL(1, queue.size());
    PacketPtr rp1 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp1 == rp1);
    LONGS_EQUAL(0, queue.size());

    expect_write(status::StatusOK, queue, wp2);

    LONGS_EQUAL(1, queue.size());
    PacketPtr rp2 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp2 == rp2);
    LONGS_EQUAL(0, queue.size());

    expect_write(status::StatusOK, queue, wp3);

    LONGS_EQUAL(1, queue.size());
    PacketPtr rp3 = expect_read(status::StatusOK, queue, ModeFetch);
    CHECK(wp3 == rp3);
    LONGS_EQUAL(0, queue.size());

    PacketPtr pp = expect_read(status::StatusDrain, queue, ModeFetch);
    CHECK(!pp);
}

TEST(sorted_queue, latest) {
    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(3);
    PacketPtr wp3 = new_packet(2);
    PacketPtr wp4 = new_packet(4);

    LONGS_EQUAL(0, queue.size());
    CHECK(!queue.latest());

    expect_write(status::StatusOK, queue, wp1);
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == wp1);

    expect_write(status::StatusOK, queue, wp2);
    LONGS_EQUAL(2, queue.size());
    CHECK(queue.latest() == wp2);

    expect_write(status::StatusOK, queue, wp3);
    LONGS_EQUAL(3, queue.size());
    CHECK(queue.latest() == wp2);

    {
        PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);
        CHECK(pp);
    }

    LONGS_EQUAL(2, queue.size());
    CHECK(queue.latest() == wp2);

    {
        PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);
        CHECK(pp);
    }

    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == wp2);

    {
        PacketPtr pp = expect_read(status::StatusOK, queue, ModeFetch);
        CHECK(pp);
    }

    LONGS_EQUAL(0, queue.size());
    CHECK(queue.latest() == wp2);

    expect_write(status::StatusOK, queue, wp4);
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == wp4);
}

} // namespace packet
} // namespace roc
