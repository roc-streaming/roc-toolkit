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

} // namespace

TEST_GROUP(sorted_queue) {};

TEST(sorted_queue, empty) {
    SortedQueue queue(0);

    CHECK(!queue.tail());
    CHECK(!queue.head());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
    CHECK(!pp);

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, two_packets) {
    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(2);

    LONGS_EQUAL(status::StatusOK, queue.write(wp2));
    LONGS_EQUAL(status::StatusOK, queue.write(wp1));

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.tail() == wp2);
    CHECK(queue.head() == wp1);

    PacketPtr rp1;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    CHECK(wp1 == rp1);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == wp2);
    CHECK(queue.head() == wp2);

    PacketPtr rp2;
    LONGS_EQUAL(status::StatusOK, queue.read(rp2));
    CHECK(wp2 == rp2);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.tail());
    CHECK(!queue.head());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
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
        LONGS_EQUAL(status::StatusOK,
                    queue.write(packets[(n + NumPackets / 2) % NumPackets]));
    }

    LONGS_EQUAL(NumPackets, queue.size());

    CHECK(queue.head() == packets[0]);
    CHECK(queue.tail() == packets[NumPackets - 1]);

    for (size_t n = 0; n < NumPackets; n++) {
        PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, queue.read(pp));
        CHECK(pp == packets[n]);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, out_of_order) {
    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(2);

    LONGS_EQUAL(status::StatusOK, queue.write(wp2));

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == wp2);
    CHECK(queue.head() == wp2);

    PacketPtr rp2;
    LONGS_EQUAL(status::StatusOK, queue.read(rp2));
    CHECK(wp2 == rp2);

    LONGS_EQUAL(0, queue.size());

    LONGS_EQUAL(status::StatusOK, queue.write(wp1));

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == wp1);
    CHECK(queue.head() == wp1);

    PacketPtr rp1;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    CHECK(wp1 == rp1);

    CHECK(!queue.tail());
    CHECK(!queue.head());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
    CHECK(!pp);
}

TEST(sorted_queue, out_of_order_many_packets) {
    enum { NumPackets = 20 };

    SortedQueue queue(0);

    for (packet::seqnum_t n = 0; n < 7; ++n) {
        LONGS_EQUAL(status::StatusOK, queue.write(new_packet(n)));
    }

    for (packet::seqnum_t n = 11; n < NumPackets; ++n) {
        LONGS_EQUAL(status::StatusOK, queue.write(new_packet(n)));
    }

    for (packet::seqnum_t n = 0; n < 7; ++n) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, queue.read(p));

        CHECK(p);
        CHECK(p->rtp()->seqnum == n);
    }

    LONGS_EQUAL(status::StatusOK, queue.write(new_packet(9)));
    LONGS_EQUAL(status::StatusOK, queue.write(new_packet(10)));

    for (packet::seqnum_t n = 9; n < NumPackets; ++n) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, queue.read(p));

        CHECK(p->rtp()->seqnum == n);

        if (n == 10) {
            LONGS_EQUAL(status::StatusOK, queue.write(new_packet(8)));
            LONGS_EQUAL(status::StatusOK, queue.write(new_packet(7)));

            LONGS_EQUAL(status::StatusOK, queue.read(p));
            LONGS_EQUAL(7, p->rtp()->seqnum);

            LONGS_EQUAL(status::StatusOK, queue.read(p));
            LONGS_EQUAL(8, p->rtp()->seqnum);
        }
    }
}

TEST(sorted_queue, one_duplicate) {
    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(1);

    LONGS_EQUAL(status::StatusOK, queue.write(wp1));
    LONGS_EQUAL(status::StatusOK, queue.write(wp2));

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == wp1);
    CHECK(queue.head() == wp1);

    PacketPtr rp1;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    CHECK(wp1 == rp1);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.tail());
    CHECK(!queue.head());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
    CHECK(!pp);
}

TEST(sorted_queue, many_duplicates) {
    const size_t NumPackets = 10;

    SortedQueue queue(0);

    for (seqnum_t n = 0; n < NumPackets; n++) {
        LONGS_EQUAL(status::StatusOK, queue.write(new_packet(n)));
    }

    LONGS_EQUAL(NumPackets, queue.size());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        LONGS_EQUAL(status::StatusOK, queue.write(new_packet(n)));
    }

    LONGS_EQUAL(NumPackets, queue.size());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr p;
        LONGS_EQUAL(status::StatusOK, queue.read(p));
        LONGS_EQUAL(n, p->rtp()->seqnum);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, max_size) {
    SortedQueue queue(2);

    PacketPtr wp1 = new_packet(1);
    PacketPtr wp2 = new_packet(2);
    PacketPtr wp3 = new_packet(3);

    LONGS_EQUAL(status::StatusOK, queue.write(wp1));
    LONGS_EQUAL(status::StatusOK, queue.write(wp2));
    LONGS_EQUAL(status::StatusOK, queue.write(wp3));

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == wp1);
    CHECK(queue.tail() == wp2);

    PacketPtr rp1;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    CHECK(wp1 == rp1);

    LONGS_EQUAL(1, queue.size());

    LONGS_EQUAL(status::StatusOK, queue.write(wp3));

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

    LONGS_EQUAL(status::StatusOK, queue.write(wp1));
    LONGS_EQUAL(status::StatusOK, queue.write(wp2));
    LONGS_EQUAL(status::StatusOK, queue.write(wp3));

    LONGS_EQUAL(3, queue.size());

    PacketPtr rp1;
    PacketPtr rp2;
    PacketPtr rp3;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    LONGS_EQUAL(status::StatusOK, queue.read(rp2));
    LONGS_EQUAL(status::StatusOK, queue.read(rp3));
    CHECK(wp1 == rp1);
    CHECK(wp2 == rp2);
    CHECK(wp3 == rp3);

    LONGS_EQUAL(0, queue.size());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
    CHECK(!pp);
}

TEST(sorted_queue, overflow_ordered2) {
    const seqnum_t sn = seqnum_t(-1) >> 1;

    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(seqnum_t(sn - 10));
    PacketPtr wp2 = new_packet(sn);
    PacketPtr wp3 = new_packet(seqnum_t(sn + 10));

    LONGS_EQUAL(status::StatusOK, queue.write(wp1));
    LONGS_EQUAL(status::StatusOK, queue.write(wp2));
    LONGS_EQUAL(status::StatusOK, queue.write(wp3));

    LONGS_EQUAL(3, queue.size());

    PacketPtr rp1;
    PacketPtr rp2;
    PacketPtr rp3;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    LONGS_EQUAL(status::StatusOK, queue.read(rp2));
    LONGS_EQUAL(status::StatusOK, queue.read(rp3));
    CHECK(wp1 == rp1);
    CHECK(wp2 == rp2);
    CHECK(wp3 == rp3);

    LONGS_EQUAL(0, queue.size());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
    CHECK(!pp);
}

TEST(sorted_queue, overflow_sorting) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(seqnum_t(sn - 10));
    PacketPtr wp2 = new_packet(sn);
    PacketPtr wp3 = new_packet(seqnum_t(sn + 10));

    LONGS_EQUAL(status::StatusOK, queue.write(wp2));
    LONGS_EQUAL(status::StatusOK, queue.write(wp1));
    LONGS_EQUAL(status::StatusOK, queue.write(wp3));

    LONGS_EQUAL(3, queue.size());

    PacketPtr rp1;
    PacketPtr rp2;
    PacketPtr rp3;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    LONGS_EQUAL(status::StatusOK, queue.read(rp2));
    LONGS_EQUAL(status::StatusOK, queue.read(rp3));
    CHECK(wp1 == rp1);
    CHECK(wp2 == rp2);
    CHECK(wp3 == rp3);

    LONGS_EQUAL(0, queue.size());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
    CHECK(!pp);
}

TEST(sorted_queue, overflow_out_of_order) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr wp1 = new_packet(seqnum_t(sn - 10));
    PacketPtr wp2 = new_packet(sn);
    PacketPtr wp3 = new_packet(sn / 2);

    LONGS_EQUAL(status::StatusOK, queue.write(wp1));

    LONGS_EQUAL(1, queue.size());
    PacketPtr rp1;
    LONGS_EQUAL(status::StatusOK, queue.read(rp1));
    CHECK(wp1 == rp1);
    LONGS_EQUAL(0, queue.size());

    LONGS_EQUAL(status::StatusOK, queue.write(wp2));

    LONGS_EQUAL(1, queue.size());
    PacketPtr rp2;
    LONGS_EQUAL(status::StatusOK, queue.read(rp2));
    CHECK(wp2 == rp2);
    LONGS_EQUAL(0, queue.size());

    LONGS_EQUAL(status::StatusOK, queue.write(wp3));

    LONGS_EQUAL(1, queue.size());
    PacketPtr rp3;
    LONGS_EQUAL(status::StatusOK, queue.read(rp3));
    CHECK(wp3 == rp3);
    LONGS_EQUAL(0, queue.size());

    PacketPtr pp;
    LONGS_EQUAL(status::StatusDrain, queue.read(pp));
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

    LONGS_EQUAL(status::StatusOK, queue.write(wp1));
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == wp1);

    LONGS_EQUAL(status::StatusOK, queue.write(wp2));
    LONGS_EQUAL(2, queue.size());
    CHECK(queue.latest() == wp2);

    LONGS_EQUAL(status::StatusOK, queue.write(wp3));
    LONGS_EQUAL(3, queue.size());
    CHECK(queue.latest() == wp2);

    {
        PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, queue.read(pp));
        CHECK(pp);
    }

    LONGS_EQUAL(2, queue.size());
    CHECK(queue.latest() == wp2);

    {
        PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, queue.read(pp));
        CHECK(pp);
    }

    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == wp2);

    {
        PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, queue.read(pp));
        CHECK(pp);
    }

    LONGS_EQUAL(0, queue.size());
    CHECK(queue.latest() == wp2);

    LONGS_EQUAL(status::StatusOK, queue.write(wp4));
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == wp4);
}

} // namespace packet
} // namespace roc
