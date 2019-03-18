/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/sorted_queue.h"

namespace roc {
namespace packet {

namespace {

core::HeapAllocator allocator;
PacketPool pool(allocator, true);

} // namespace

TEST_GROUP(sorted_queue) {
    PacketPtr new_packet(seqnum_t sn) {
        PacketPtr packet = new(pool) Packet(pool);
        CHECK(packet);

        packet->add_flags(Packet::FlagRTP);
        packet->rtp()->seqnum = sn;

        return packet;
    }
};

TEST(sorted_queue, empty) {
    SortedQueue queue(0);

    CHECK(!queue.tail());
    CHECK(!queue.head());

    CHECK(!queue.read());

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, two_packets) {
    SortedQueue queue(0);

    PacketPtr p1 = new_packet(1);
    PacketPtr p2 = new_packet(2);

    queue.write(p2);
    queue.write(p1);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.tail() == p2);
    CHECK(queue.head() == p1);

    CHECK(queue.read() == p1);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == p2);
    CHECK(queue.head() == p2);

    CHECK(queue.read() == p2);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.tail());
    CHECK(!queue.head());

    CHECK(!queue.read());

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
        queue.write(packets[(n + NumPackets / 2) % NumPackets]);
    }

    LONGS_EQUAL(NumPackets, queue.size());

    CHECK(queue.head() == packets[0]);
    CHECK(queue.tail() == packets[NumPackets - 1]);

    for (size_t n = 0; n < NumPackets; n++) {
        CHECK(queue.read() == packets[n]);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, out_of_order) {
    SortedQueue queue(0);

    PacketPtr p1 = new_packet(1);
    PacketPtr p2 = new_packet(2);

    queue.write(p2);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == p2);
    CHECK(queue.head() == p2);

    CHECK(queue.read() == p2);

    LONGS_EQUAL(0, queue.size());

    queue.write(p1);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == p1);
    CHECK(queue.head() == p1);

    CHECK(queue.read() == p1);

    CHECK(!queue.tail());
    CHECK(!queue.head());

    CHECK(!queue.read());
}

TEST(sorted_queue, out_of_order_many_packets) {
    enum { NumPackets = 20 };

    SortedQueue queue(0);

    for (packet::seqnum_t n = 0; n < 7; ++n) {
        queue.write(new_packet(n));
    }

    for (packet::seqnum_t n = 11; n < NumPackets; ++n) {
        queue.write(new_packet(n));
    }

    for (packet::seqnum_t n = 0; n < 7; ++n) {
        const packet::PacketPtr p = queue.read();

        CHECK(p);
        CHECK(p->rtp()->seqnum == n);
    }

    queue.write(new_packet(9));
    queue.write(new_packet(10));

    for (packet::seqnum_t n = 9; n < NumPackets; ++n) {
        const packet::PacketPtr p = queue.read();

        CHECK(p->rtp()->seqnum == n);

        if (n == 10) {
            queue.write(new_packet(8));
            queue.write(new_packet(7));

            CHECK(queue.read()->rtp()->seqnum == 7);
            CHECK(queue.read()->rtp()->seqnum == 8);
        }
    }
}

TEST(sorted_queue, one_duplicate) {
    SortedQueue queue(0);

    PacketPtr p1 = new_packet(1);
    PacketPtr p2 = new_packet(1);

    queue.write(p1);
    queue.write(p2);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.tail() == p1);
    CHECK(queue.head() == p1);

    CHECK(queue.read() == p1);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.tail());
    CHECK(!queue.head());

    CHECK(!queue.read());
}

TEST(sorted_queue, many_duplicates) {
    const size_t NumPackets = 10;

    SortedQueue queue(0);

    for (seqnum_t n = 0; n < NumPackets; n++) {
        queue.write(new_packet(n));
    }

    LONGS_EQUAL(NumPackets, queue.size());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        queue.write(new_packet(n));
    }

    LONGS_EQUAL(NumPackets, queue.size());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(queue.read()->rtp()->seqnum == n);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(sorted_queue, max_size) {
    SortedQueue queue(2);

    PacketPtr p1 = new_packet(1);
    PacketPtr p2 = new_packet(2);
    PacketPtr p3 = new_packet(3);

    queue.write(p1);
    queue.write(p2);
    queue.write(p3);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == p1);
    CHECK(queue.tail() == p2);

    CHECK(queue.read() == p1);

    LONGS_EQUAL(1, queue.size());

    queue.write(p3);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.head() == p2);
    CHECK(queue.tail() == p3);
}

TEST(sorted_queue, overflow_ordered1) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr p1 = new_packet(seqnum_t(sn - 10));
    PacketPtr p2 = new_packet(sn);
    PacketPtr p3 = new_packet(seqnum_t(sn + 10));

    queue.write(p1);
    queue.write(p2);
    queue.write(p3);

    LONGS_EQUAL(3, queue.size());

    CHECK(queue.read() == p1);
    CHECK(queue.read() == p2);
    CHECK(queue.read() == p3);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.read());
}

TEST(sorted_queue, overflow_ordered2) {
    const seqnum_t sn = seqnum_t(-1) >> 1;

    SortedQueue queue(0);

    PacketPtr p1 = new_packet(seqnum_t(sn - 10));
    PacketPtr p2 = new_packet(sn);
    PacketPtr p3 = new_packet(seqnum_t(sn + 10));

    queue.write(p1);
    queue.write(p2);
    queue.write(p3);

    LONGS_EQUAL(3, queue.size());

    CHECK(queue.read() == p1);
    CHECK(queue.read() == p2);
    CHECK(queue.read() == p3);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.read());
}

TEST(sorted_queue, overflow_sorting) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr p1 = new_packet(seqnum_t(sn - 10));
    PacketPtr p2 = new_packet(sn);
    PacketPtr p3 = new_packet(seqnum_t(sn + 10));

    queue.write(p2);
    queue.write(p1);
    queue.write(p3);

    LONGS_EQUAL(3, queue.size());

    CHECK(queue.read() == p1);
    CHECK(queue.read() == p2);
    CHECK(queue.read() == p3);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.read());
}

TEST(sorted_queue, overflow_out_of_order) {
    const seqnum_t sn = seqnum_t(-1);

    SortedQueue queue(0);

    PacketPtr p1 = new_packet(seqnum_t(sn - 10));
    PacketPtr p2 = new_packet(sn);
    PacketPtr p3 = new_packet(sn / 2);

    queue.write(p1);

    LONGS_EQUAL(1, queue.size());
    CHECK(queue.read() == p1);
    LONGS_EQUAL(0, queue.size());

    queue.write(p2);

    LONGS_EQUAL(1, queue.size());
    CHECK(queue.read() == p2);
    LONGS_EQUAL(0, queue.size());

    queue.write(p3);

    LONGS_EQUAL(1, queue.size());
    CHECK(queue.read() == p3);
    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.read());
}

TEST(sorted_queue, latest) {
    SortedQueue queue(0);

    PacketPtr p1 = new_packet(1);
    PacketPtr p2 = new_packet(3);
    PacketPtr p3 = new_packet(2);
    PacketPtr p4 = new_packet(4);

    LONGS_EQUAL(0, queue.size());
    CHECK(!queue.latest());

    queue.write(p1);
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == p1);

    queue.write(p2);
    LONGS_EQUAL(2, queue.size());
    CHECK(queue.latest() == p2);

    queue.write(p3);
    LONGS_EQUAL(3, queue.size());
    CHECK(queue.latest() == p2);

    CHECK(queue.read());
    LONGS_EQUAL(2, queue.size());
    CHECK(queue.latest() == p2);

    CHECK(queue.read());
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == p2);

    CHECK(queue.read());
    LONGS_EQUAL(0, queue.size());
    CHECK(queue.latest() == p2);

    queue.write(p4);
    LONGS_EQUAL(1, queue.size());
    CHECK(queue.latest() == p4);
}

} // namespace packet
} // namespace roc
