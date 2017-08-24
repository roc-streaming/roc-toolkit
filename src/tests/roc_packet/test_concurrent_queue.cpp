/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace packet {

namespace {

core::HeapAllocator allocator;
PacketPool pool(allocator, 1);

} // namespace

TEST_GROUP(concurrent_queue) {
    PacketPtr new_packet() {
        PacketPtr packet = new(pool) Packet(pool);
        CHECK(packet);
        return packet;
    }
};

TEST(concurrent_queue, empty) {
    ConcurrentQueue queue(0, false);

    CHECK(!queue.read());

    LONGS_EQUAL(0, queue.size());
}

TEST(concurrent_queue, two_packets) {
    ConcurrentQueue queue(0, false);

    PacketPtr p1 = new_packet();
    PacketPtr p2 = new_packet();

    queue.write(p1);
    queue.write(p2);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.read() == p1);

    LONGS_EQUAL(1, queue.size());

    CHECK(queue.read() == p2);

    LONGS_EQUAL(0, queue.size());

    CHECK(!queue.read());

    LONGS_EQUAL(0, queue.size());
}

TEST(concurrent_queue, many_packets) {
    enum { NumPackets = 10 };

    ConcurrentQueue queue(0, false);

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet();
    }

    for (ssize_t n = 0; n < NumPackets; n++) {
        queue.write(packets[n]);
    }

    LONGS_EQUAL(NumPackets, queue.size());

    for (size_t n = 0; n < NumPackets; n++) {
        CHECK(queue.read() == packets[n]);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(concurrent_queue, max_size) {
    ConcurrentQueue queue(2, false);

    PacketPtr p1 = new_packet();
    PacketPtr p2 = new_packet();
    PacketPtr p3 = new_packet();

    queue.write(p1);
    queue.write(p2);
    queue.write(p3);

    LONGS_EQUAL(2, queue.size());

    CHECK(queue.read() == p1);

    LONGS_EQUAL(1, queue.size());

    queue.write(p3);

    LONGS_EQUAL(2, queue.size());
}

TEST(concurrent_queue, blocking) {
    ConcurrentQueue queue(0, true);

    PacketPtr p = new_packet();

    queue.write(p);

    queue.wait();

    CHECK(queue.read() == p);
}

} // namespace packet
} // namespace roc
