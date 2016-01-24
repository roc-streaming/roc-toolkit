/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/packet_queue.h"

#include "test_packet.h"

namespace roc {
namespace test {

using namespace packet;

TEST_GROUP(packet_queue) {
    IPacketPtr new_packet(seqnum_t sn) {
        return new_audio_packet(0, sn, 0);
    }
};

TEST(packet_queue, empty) {
    PacketQueue queue;

    CHECK(!queue.tail());
    CHECK(!queue.head());

    CHECK(!queue.read());

    LONGS_EQUAL(0, queue.size());
}

TEST(packet_queue, two_packets) {
    PacketQueue queue;

    IPacketPtr p1 = new_packet(1);
    IPacketPtr p2 = new_packet(2);

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

TEST(packet_queue, many_packets) {
    const size_t num_packets = 10;

    PacketQueue queue;

    IPacketPtr packets[num_packets];

    for (seqnum_t n = 0; n < num_packets; n++) {
        packets[n] = new_packet(n);
    }

    for (ssize_t n = ssize_t(num_packets) - 1; n >= 0; n--) {
        queue.write(packets[n]);
    }

    LONGS_EQUAL(num_packets, queue.size());

    CHECK(queue.head() == packets[0]);
    CHECK(queue.tail() == packets[num_packets - 1]);

    for (size_t n = 0; n < num_packets; n++) {
        CHECK(queue.read() == packets[n]);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(packet_queue, out_of_order) {
    PacketQueue queue;

    IPacketPtr p1 = new_packet(1);
    IPacketPtr p2 = new_packet(2);

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

TEST(packet_queue, one_duplicate) {
    PacketQueue queue;

    IPacketPtr p1 = new_packet(1);
    IPacketPtr p2 = new_packet(1);

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

TEST(packet_queue, many_duplicates) {
    const size_t num_packets = 10;

    PacketQueue queue;

    for (seqnum_t n = 0; n < num_packets; n++) {
        queue.write(new_packet(n));
    }

    LONGS_EQUAL(num_packets, queue.size());

    for (seqnum_t n = 0; n < num_packets; n++) {
        queue.write(new_packet(n));
    }

    LONGS_EQUAL(num_packets, queue.size());

    for (seqnum_t n = 0; n < num_packets; n++) {
        CHECK(queue.read()->rtp()->seqnum() == n);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(packet_queue, max_size) {
    PacketQueue queue(2);

    IPacketPtr p1 = new_packet(1);
    IPacketPtr p2 = new_packet(2);
    IPacketPtr p3 = new_packet(3);

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

TEST(packet_queue, overflow_ordered1) {
    const seqnum_t sn = seqnum_t(-1);

    PacketQueue queue;

    IPacketPtr p1 = new_packet(seqnum_t(sn - 10));
    IPacketPtr p2 = new_packet(sn);
    IPacketPtr p3 = new_packet(seqnum_t(sn + 10));

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

TEST(packet_queue, overflow_ordered2) {
    const seqnum_t sn = seqnum_t(-1) >> 1;

    PacketQueue queue;

    IPacketPtr p1 = new_packet(seqnum_t(sn - 10));
    IPacketPtr p2 = new_packet(sn);
    IPacketPtr p3 = new_packet(seqnum_t(sn + 10));

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

TEST(packet_queue, overflow_sorting) {
    const seqnum_t sn = seqnum_t(-1);

    PacketQueue queue;

    IPacketPtr p1 = new_packet(seqnum_t(sn - 10));
    IPacketPtr p2 = new_packet(sn);
    IPacketPtr p3 = new_packet(seqnum_t(sn + 10));

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

TEST(packet_queue, overflow_out_of_order) {
    const seqnum_t sn = seqnum_t(-1);

    PacketQueue queue;

    IPacketPtr p1 = new_packet(seqnum_t(sn - 10));
    IPacketPtr p2 = new_packet(sn);
    IPacketPtr p3 = new_packet(sn / 2);

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

} // namespace test
} // namespace roc
