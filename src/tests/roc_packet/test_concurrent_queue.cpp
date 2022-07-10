/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace packet {

namespace {

core::HeapAllocator allocator;
PacketFactory packet_factory(allocator, true);

PacketPtr new_packet() {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);
    return packet;
}

} // namespace

TEST_GROUP(concurrent_queue) {};

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
