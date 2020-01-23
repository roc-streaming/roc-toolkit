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
#include "roc_packet/queue.h"
#include "roc_packet/router.h"

namespace roc {
namespace packet {

namespace {

core::HeapAllocator allocator;
PacketPool pool(allocator, true);

} // namespace

TEST_GROUP(router) {
    PacketPtr new_packet(source_t source, unsigned flags) {
        PacketPtr packet = new(pool) Packet(pool);
        CHECK(packet);
        packet->add_flags(Packet::FlagRTP | flags);
        packet->rtp()->source = source;
        return packet;
    }
};

TEST(router, no_routes) {
    Router router(allocator);

    PacketPtr p = new_packet(0, Packet::FlagAudio);

    router.write(p);

    LONGS_EQUAL(1, p->getref());
}

TEST(router, one_route) {
    Router router(allocator);

    Queue queue;
    CHECK(router.add_route(queue, Packet::FlagAudio));

    PacketPtr pa1 = new_packet(0, Packet::FlagAudio);
    PacketPtr pa2 = new_packet(0, Packet::FlagAudio);

    PacketPtr pf1 = new_packet(0, Packet::FlagFEC);
    PacketPtr pf2 = new_packet(0, Packet::FlagFEC);

    router.write(pa1);
    router.write(pa2);

    router.write(pf1);
    router.write(pf2);

    LONGS_EQUAL(2, pa1->getref());
    LONGS_EQUAL(2, pa2->getref());

    LONGS_EQUAL(1, pf1->getref());
    LONGS_EQUAL(1, pf2->getref());

    CHECK(queue.read() == pa1);
    CHECK(queue.read() == pa2);
    CHECK(!queue.read());
}

TEST(router, two_routes) {
    Router router(allocator);

    Queue queue_a;
    CHECK(router.add_route(queue_a, Packet::FlagAudio));

    Queue queue_f;
    CHECK(router.add_route(queue_f, Packet::FlagFEC));

    PacketPtr pa1 = new_packet(0, Packet::FlagAudio);
    PacketPtr pa2 = new_packet(0, Packet::FlagAudio);

    PacketPtr pf1 = new_packet(0, Packet::FlagFEC);
    PacketPtr pf2 = new_packet(0, Packet::FlagFEC);

    router.write(pa1);
    router.write(pa2);

    router.write(pf1);
    router.write(pf2);

    LONGS_EQUAL(2, pa1->getref());
    LONGS_EQUAL(2, pa2->getref());

    LONGS_EQUAL(2, pf1->getref());
    LONGS_EQUAL(2, pf2->getref());

    CHECK(queue_a.read() == pa1);
    CHECK(queue_a.read() == pa2);
    CHECK(!queue_a.read());

    CHECK(queue_f.read() == pf1);
    CHECK(queue_f.read() == pf2);
    CHECK(!queue_f.read());
}

TEST(router, same_route_different_sources) {
    Router router(allocator);

    Queue queue;
    CHECK(router.add_route(queue, Packet::FlagAudio));

    router.write(new_packet(11, Packet::FlagAudio));
    UNSIGNED_LONGS_EQUAL(1, queue.size());

    router.write(new_packet(22, Packet::FlagAudio));
    UNSIGNED_LONGS_EQUAL(1, queue.size());

    router.write(new_packet(11, Packet::FlagAudio));
    UNSIGNED_LONGS_EQUAL(2, queue.size());
}

TEST(router, different_routes_same_source) {
    Router router(allocator);

    Queue queue_a;
    CHECK(router.add_route(queue_a, Packet::FlagAudio));

    Queue queue_f;
    CHECK(router.add_route(queue_f, Packet::FlagFEC));

    router.write(new_packet(11, Packet::FlagAudio));
    router.write(new_packet(11, Packet::FlagFEC));

    UNSIGNED_LONGS_EQUAL(1, queue_a.size());
    UNSIGNED_LONGS_EQUAL(1, queue_f.size());
}

TEST(router, different_routes_different_sources) {
    Router router(allocator);

    Queue queue_a;
    CHECK(router.add_route(queue_a, Packet::FlagAudio));

    Queue queue_f;
    CHECK(router.add_route(queue_f, Packet::FlagFEC));

    router.write(new_packet(11, Packet::FlagAudio));
    router.write(new_packet(22, Packet::FlagFEC));

    UNSIGNED_LONGS_EQUAL(1, queue_a.size());
    UNSIGNED_LONGS_EQUAL(1, queue_f.size());
}

} // namespace packet
} // namespace roc
