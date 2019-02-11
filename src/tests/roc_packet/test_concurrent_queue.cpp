/*
 * Copyright (c) 2015 Roc authors
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
PacketPool pool(allocator, true);

} // namespace

TEST_GROUP(concurrent_queue) {
    PacketPtr new_packet() {
        PacketPtr packet = new(pool) Packet(pool);
        CHECK(packet);
        return packet;
    }
};

TEST(concurrent_queue, write_read) {
    ConcurrentQueue queue;

    PacketPtr p1 = new_packet();
    PacketPtr p2 = new_packet();

    queue.write(p1);
    queue.write(p2);

    CHECK(queue.read() == p1);
    CHECK(queue.read() == p2);
}

} // namespace packet
} // namespace roc
