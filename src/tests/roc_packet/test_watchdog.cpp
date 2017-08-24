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
#include "roc_packet/watchdog.h"

namespace roc {
namespace packet {

namespace {

core::HeapAllocator allocator;
PacketPool pool(allocator, 1);

} // namespace

TEST_GROUP(watchdog) {
    PacketPtr new_packet() {
        PacketPtr packet = new(pool) Packet(pool);
        CHECK(packet);
        return packet;
    }
};

TEST(watchdog, no_packets) {
    enum { Timeout = 20 };

    ConcurrentQueue queue(0, false);
    Watchdog watchdog(queue, Timeout);

    CHECK(watchdog.update(0));
    CHECK(!watchdog.read());
}

TEST(watchdog, read) {
    enum { Timeout = 20, NumPackets = Timeout + 5 };

    ConcurrentQueue queue(0, false);
    Watchdog watchdog(queue, Timeout);

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet();
        queue.write(packets[n]);
    }

    for (timestamp_t n = 0; n < NumPackets; n++) {
        CHECK(watchdog.update(n));
        CHECK(watchdog.read() == packets[n]);
    }

    CHECK(watchdog.update(NumPackets));
    CHECK(!watchdog.read());
}

TEST(watchdog, timeout) {
    enum { Timeout = 20, NumPackets = Timeout - 5, Offset = 10000 };

    ConcurrentQueue queue(0, false);
    Watchdog watchdog(queue, Timeout);

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr packet = new_packet();
        queue.write(packet);

        CHECK(watchdog.update(Offset + n));
        CHECK(watchdog.read() == packet);
    }

    for (timestamp_t n = NumPackets; n < NumPackets + Timeout - 1; n++) {
        CHECK(watchdog.update(Offset + n));
        CHECK(!watchdog.read());
    }

    PacketPtr packet = new_packet();
    queue.write(packet);

    CHECK(!watchdog.update(Offset + NumPackets + Timeout));
    CHECK(!watchdog.read());
}

} // namespace packet
} // namespace roc
