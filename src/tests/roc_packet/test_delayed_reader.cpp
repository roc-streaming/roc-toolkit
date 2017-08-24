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
#include "roc_packet/delayed_reader.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace packet {

namespace {

enum { NumSamples = 100, NumPackets = 5 };

core::HeapAllocator allocator;
PacketPool pool(allocator, 1);

} // namespace

TEST_GROUP(delayed_reader) {
    PacketPtr new_packet(seqnum_t sn) {
        PacketPtr packet = new(pool) Packet(pool);
        CHECK(packet);

        packet->add_flags(Packet::FlagRTP);
        packet->rtp()->seqnum = sn;
        packet->rtp()->timestamp = timestamp_t(sn * NumSamples);

        return packet;
    }
};

TEST(delayed_reader, no_delay) {
    ConcurrentQueue queue(0, false);
    DelayedReader dr(queue, 0);

    CHECK(!dr.read());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr packet = new_packet(n);
        queue.write(packet);
        CHECK(dr.read() == packet);
    }
}

TEST(delayed_reader, delay1) {
    ConcurrentQueue queue(0, false);
    DelayedReader dr(queue, NumSamples * (NumPackets - 1));

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(!dr.read());
        packets[n] = new_packet(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(dr.read() == packets[n]);
    }

    CHECK(!dr.read());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr packet = new_packet(NumPackets + n);
        queue.write(packet);
        CHECK(dr.read() == packet);
    }

    CHECK(!dr.read());
}

TEST(delayed_reader, delay2) {
    ConcurrentQueue queue(0, false);
    DelayedReader dr(queue, NumSamples * (NumPackets - 1));

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(dr.read() == packets[n]);
    }

    CHECK(!dr.read());
}

TEST(delayed_reader, late_duplicates) {
    ConcurrentQueue queue(0, false);
    DelayedReader dr(queue, NumSamples * (NumPackets - 1));

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(dr.read() == packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr packet = new_packet(n);
        queue.write(packet);
        CHECK(dr.read() == packet);
    }

    CHECK(!dr.read());
}

} // namespace packet
} // namespace roc
