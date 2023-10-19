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
#include "roc_packet/queue.h"
#include "roc_packet/router.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

namespace {

core::HeapArena arena;
PacketFactory packet_factory(arena);

PacketPtr new_packet(stream_source_t source, unsigned flags) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);
    packet->add_flags(Packet::FlagRTP | flags);
    packet->rtp()->source = source;
    return packet;
}

} // namespace

TEST_GROUP(router) {};

TEST(router, no_routes) {
    Router router(arena);

    PacketPtr p = new_packet(0, Packet::FlagAudio);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(p));

    LONGS_EQUAL(1, p->getref());
}

TEST(router, one_route) {
    Router router(arena);

    Queue queue;
    CHECK(router.add_route(queue, Packet::FlagAudio));

    PacketPtr wpa1 = new_packet(0, Packet::FlagAudio);
    PacketPtr wpa2 = new_packet(0, Packet::FlagAudio);

    PacketPtr pf1 = new_packet(0, Packet::FlagFEC);
    PacketPtr pf2 = new_packet(0, Packet::FlagFEC);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(wpa1));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(wpa2));

    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(pf1));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(pf2));

    LONGS_EQUAL(2, wpa1->getref());
    LONGS_EQUAL(2, wpa2->getref());

    LONGS_EQUAL(1, pf1->getref());
    LONGS_EQUAL(1, pf2->getref());

    PacketPtr rpa1;
    PacketPtr rpa2;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rpa1));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rpa2));
    CHECK(wpa1 == rpa1);
    CHECK(wpa2 == rpa2);

    PacketPtr pp;
    LONGS_EQUAL(status::StatusNoData, queue.read(pp));
    CHECK(!pp);
}

TEST(router, two_routes) {
    Router router(arena);

    Queue queue_a;
    CHECK(router.add_route(queue_a, Packet::FlagAudio));

    Queue queue_f;
    CHECK(router.add_route(queue_f, Packet::FlagFEC));

    PacketPtr wpa1 = new_packet(0, Packet::FlagAudio);
    PacketPtr wpa2 = new_packet(0, Packet::FlagAudio);

    PacketPtr wpf1 = new_packet(0, Packet::FlagFEC);
    PacketPtr wpf2 = new_packet(0, Packet::FlagFEC);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(wpa1));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(wpa2));

    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(wpf1));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(wpf2));

    LONGS_EQUAL(2, wpa1->getref());
    LONGS_EQUAL(2, wpa2->getref());

    LONGS_EQUAL(2, wpf1->getref());
    LONGS_EQUAL(2, wpf2->getref());

    PacketPtr rpa1;
    PacketPtr rpa2;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue_a.read(rpa1));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue_a.read(rpa2));
    CHECK(wpa1 == rpa1);
    CHECK(wpa2 == rpa2);

    PacketPtr ppa;
    LONGS_EQUAL(status::StatusNoData, queue_a.read(ppa));
    CHECK(!ppa);

    PacketPtr rpf1;
    PacketPtr rpf2;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue_f.read(rpf1));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue_f.read(rpf2));
    CHECK(wpf1 == rpf1);
    CHECK(wpf2 == rpf2);

    PacketPtr ppf;
    CHECK(queue_f.read(ppa) == status::StatusNoData);
    CHECK(!ppf);
}

TEST(router, same_route_different_sources) {
    Router router(arena);

    Queue queue;
    CHECK(router.add_route(queue, Packet::FlagAudio));

    UNSIGNED_LONGS_EQUAL(status::StatusOK,
                         router.write(new_packet(11, Packet::FlagAudio)));
    UNSIGNED_LONGS_EQUAL(1, queue.size());

    UNSIGNED_LONGS_EQUAL(status::StatusOK,
                         router.write(new_packet(22, Packet::FlagAudio)));
    UNSIGNED_LONGS_EQUAL(1, queue.size());

    UNSIGNED_LONGS_EQUAL(status::StatusOK,
                         router.write(new_packet(11, Packet::FlagAudio)));
    UNSIGNED_LONGS_EQUAL(2, queue.size());
}

TEST(router, different_routes_same_source) {
    Router router(arena);

    Queue queue_a;
    CHECK(router.add_route(queue_a, Packet::FlagAudio));

    Queue queue_f;
    CHECK(router.add_route(queue_f, Packet::FlagFEC));

    UNSIGNED_LONGS_EQUAL(status::StatusOK,
                         router.write(new_packet(11, Packet::FlagAudio)));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(new_packet(11, Packet::FlagFEC)));

    UNSIGNED_LONGS_EQUAL(1, queue_a.size());
    UNSIGNED_LONGS_EQUAL(1, queue_f.size());
}

TEST(router, different_routes_different_sources) {
    Router router(arena);

    Queue queue_a;
    CHECK(router.add_route(queue_a, Packet::FlagAudio));

    Queue queue_f;
    CHECK(router.add_route(queue_f, Packet::FlagFEC));

    UNSIGNED_LONGS_EQUAL(status::StatusOK,
                         router.write(new_packet(11, Packet::FlagAudio)));
    UNSIGNED_LONGS_EQUAL(status::StatusOK, router.write(new_packet(22, Packet::FlagFEC)));

    UNSIGNED_LONGS_EQUAL(1, queue_a.size());
    UNSIGNED_LONGS_EQUAL(1, queue_f.size());
}

} // namespace packet
} // namespace roc
