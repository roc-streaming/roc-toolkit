/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/router.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;
PacketFactory packet_factory(arena, MaxBufSize);

PacketPtr new_rtp_packet(stream_source_t source, unsigned flags) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);
    packet->add_flags(Packet::FlagRTP | flags);
    packet->rtp()->source_id = source;
    return packet;
}

PacketPtr new_fec_packet(unsigned flags) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);
    packet->add_flags(Packet::FlagFEC | flags);
    return packet;
}

} // namespace

TEST_GROUP(router) {};

TEST(router, no_routes) {
    Router router(arena);

    PacketPtr p = new_rtp_packet(11, Packet::FlagAudio);

    LONGS_EQUAL(status::StatusNoRoute, router.write(p));

    LONGS_EQUAL(1, p->getref());
}

TEST(router, one_route) {
    Router router(arena);

    FifoQueue queue;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagAudio));

    PacketPtr wpa1 = new_rtp_packet(11, Packet::FlagAudio);
    PacketPtr wpa2 = new_rtp_packet(11, Packet::FlagAudio);

    PacketPtr pf1 = new_fec_packet(Packet::FlagRepair);
    PacketPtr pf2 = new_fec_packet(Packet::FlagRepair);

    LONGS_EQUAL(status::StatusOK, router.write(wpa1));
    LONGS_EQUAL(status::StatusOK, router.write(wpa2));

    LONGS_EQUAL(status::StatusNoRoute, router.write(pf1));
    LONGS_EQUAL(status::StatusNoRoute, router.write(pf2));

    LONGS_EQUAL(2, wpa1->getref());
    LONGS_EQUAL(2, wpa2->getref());

    LONGS_EQUAL(1, pf1->getref());
    LONGS_EQUAL(1, pf2->getref());

    PacketPtr rpa1;
    PacketPtr rpa2;
    LONGS_EQUAL(status::StatusOK, queue.read(rpa1, ModeFetch));
    LONGS_EQUAL(status::StatusOK, queue.read(rpa2, ModeFetch));
    CHECK(wpa1 == rpa1);
    CHECK(wpa2 == rpa2);

    LONGS_EQUAL(0, queue.size());
}

TEST(router, two_routes) {
    Router router(arena);

    FifoQueue queue_a;
    FifoQueue queue_r;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_a, Packet::FlagAudio));
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_r, Packet::FlagRepair));

    PacketPtr wpa1 = new_rtp_packet(11, Packet::FlagAudio);
    PacketPtr wpa2 = new_rtp_packet(11, Packet::FlagAudio);

    PacketPtr wpr1 = new_fec_packet(Packet::FlagRepair);
    PacketPtr wpr2 = new_fec_packet(Packet::FlagRepair);

    LONGS_EQUAL(status::StatusOK, router.write(wpa1));
    LONGS_EQUAL(status::StatusOK, router.write(wpa2));

    LONGS_EQUAL(status::StatusOK, router.write(wpr1));
    LONGS_EQUAL(status::StatusOK, router.write(wpr2));

    LONGS_EQUAL(2, wpa1->getref());
    LONGS_EQUAL(2, wpa2->getref());

    LONGS_EQUAL(2, wpr1->getref());
    LONGS_EQUAL(2, wpr2->getref());

    PacketPtr rpa1;
    PacketPtr rpa2;
    LONGS_EQUAL(status::StatusOK, queue_a.read(rpa1, ModeFetch));
    LONGS_EQUAL(status::StatusOK, queue_a.read(rpa2, ModeFetch));
    CHECK(wpa1 == rpa1);
    CHECK(wpa2 == rpa2);

    PacketPtr rpr1;
    PacketPtr rpr2;
    LONGS_EQUAL(status::StatusOK, queue_r.read(rpr1, ModeFetch));
    LONGS_EQUAL(status::StatusOK, queue_r.read(rpr2, ModeFetch));
    CHECK(wpr1 == rpr1);
    CHECK(wpr2 == rpr2);

    LONGS_EQUAL(0, queue_a.size());
    LONGS_EQUAL(0, queue_r.size());
}

TEST(router, two_routes_two_sources) {
    Router router(arena);

    FifoQueue queue_a;
    FifoQueue queue_r;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_a, Packet::FlagAudio));
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_r, Packet::FlagRepair));

    PacketPtr wpa1 = new_rtp_packet(11, Packet::FlagAudio);
    PacketPtr wpa2 = new_rtp_packet(11, Packet::FlagAudio);

    PacketPtr wpr1 = new_rtp_packet(22, Packet::FlagRepair);
    PacketPtr wpr2 = new_rtp_packet(22, Packet::FlagRepair);

    LONGS_EQUAL(status::StatusOK, router.write(wpa1));
    LONGS_EQUAL(status::StatusOK, router.write(wpa2));

    LONGS_EQUAL(status::StatusOK, router.write(wpr1));
    LONGS_EQUAL(status::StatusOK, router.write(wpr2));

    LONGS_EQUAL(2, wpa1->getref());
    LONGS_EQUAL(2, wpa2->getref());

    LONGS_EQUAL(2, wpr1->getref());
    LONGS_EQUAL(2, wpr2->getref());

    PacketPtr rpa1;
    PacketPtr rpa2;
    LONGS_EQUAL(status::StatusOK, queue_a.read(rpa1, ModeFetch));
    LONGS_EQUAL(status::StatusOK, queue_a.read(rpa2, ModeFetch));
    CHECK(wpa1 == rpa1);
    CHECK(wpa2 == rpa2);

    PacketPtr rpr1;
    PacketPtr rpr2;
    LONGS_EQUAL(status::StatusOK, queue_r.read(rpr1, ModeFetch));
    LONGS_EQUAL(status::StatusOK, queue_r.read(rpr2, ModeFetch));
    CHECK(wpr1 == rpr1);
    CHECK(wpr2 == rpr2);

    LONGS_EQUAL(0, queue_a.size());
    LONGS_EQUAL(0, queue_r.size());
}

TEST(router, same_route_different_sources) {
    Router router(arena);

    FifoQueue queue;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagAudio));

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(1, queue.size());

    // Dropped because have different source.
    LONGS_EQUAL(status::StatusNoRoute,
                router.write(new_rtp_packet(22, Packet::FlagAudio)));
    LONGS_EQUAL(1, queue.size());

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(2, queue.size());
}

TEST(router, different_routes_same_source) {
    Router router(arena);

    FifoQueue queue_a;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_a, Packet::FlagAudio));

    FifoQueue queue_r;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_r, Packet::FlagRepair));

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagRepair)));

    LONGS_EQUAL(1, queue_a.size());
    LONGS_EQUAL(1, queue_r.size());
}

TEST(router, different_routes_different_sources) {
    Router router(arena);

    FifoQueue queue_a;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_a, Packet::FlagAudio));

    FifoQueue queue_r;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue_r, Packet::FlagRepair));

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(22, Packet::FlagRepair)));

    LONGS_EQUAL(1, queue_a.size());
    LONGS_EQUAL(1, queue_r.size());
}

TEST(router, same_route_first_without_source_then_with_source) {
    Router router(arena);

    FifoQueue queue;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagRepair));

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagRepair)));
    LONGS_EQUAL(1, queue.size());

    // Dropped because route has source, and packet doesn't.
    LONGS_EQUAL(status::StatusNoRoute, router.write(new_fec_packet(Packet::FlagRepair)));
    LONGS_EQUAL(1, queue.size());

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagRepair)));
    LONGS_EQUAL(2, queue.size());
}

TEST(router, same_route_first_with_source_then_without_source) {
    Router router(arena);

    FifoQueue queue;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagRepair));

    LONGS_EQUAL(status::StatusOK, router.write(new_fec_packet(Packet::FlagRepair)));
    LONGS_EQUAL(1, queue.size());

    // Dropped because route doesn't have source, and packet has.
    LONGS_EQUAL(status::StatusNoRoute,
                router.write(new_rtp_packet(11, Packet::FlagRepair)));
    LONGS_EQUAL(1, queue.size());

    LONGS_EQUAL(status::StatusOK, router.write(new_fec_packet(Packet::FlagRepair)));
    LONGS_EQUAL(2, queue.size());
}

TEST(router, source_id_one_source) {
    Router router(arena);

    CHECK(!router.has_source_id(Packet::FlagAudio));
    CHECK(!router.has_source_id(Packet::FlagRepair));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagAudio));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagRepair));

    FifoQueue queue;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagAudio));
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagRepair));

    CHECK(!router.has_source_id(Packet::FlagAudio));
    CHECK(!router.has_source_id(Packet::FlagRepair));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagAudio));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagRepair));

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(status::StatusOK, router.write(new_fec_packet(Packet::FlagRepair)));
    LONGS_EQUAL(status::StatusOK, router.write(new_fec_packet(Packet::FlagRepair)));
    LONGS_EQUAL(4, queue.size());

    CHECK(router.has_source_id(Packet::FlagAudio));
    CHECK(!router.has_source_id(Packet::FlagRepair));
    LONGS_EQUAL(11, router.get_source_id(Packet::FlagAudio));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagRepair));
}

TEST(router, source_id_two_sources) {
    Router router(arena);

    CHECK(!router.has_source_id(Packet::FlagAudio));
    CHECK(!router.has_source_id(Packet::FlagRepair));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagAudio));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagRepair));

    FifoQueue queue;
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagAudio));
    LONGS_EQUAL(status::StatusOK, router.add_route(queue, Packet::FlagRepair));

    CHECK(!router.has_source_id(Packet::FlagAudio));
    CHECK(!router.has_source_id(Packet::FlagRepair));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagAudio));
    LONGS_EQUAL(0, router.get_source_id(Packet::FlagRepair));

    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(11, Packet::FlagAudio)));
    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(22, Packet::FlagRepair)));
    LONGS_EQUAL(status::StatusOK, router.write(new_rtp_packet(22, Packet::FlagRepair)));
    LONGS_EQUAL(4, queue.size());

    CHECK(router.has_source_id(Packet::FlagAudio));
    CHECK(router.has_source_id(Packet::FlagRepair));
    LONGS_EQUAL(11, router.get_source_id(Packet::FlagAudio));
    LONGS_EQUAL(22, router.get_source_id(Packet::FlagRepair));
}

} // namespace packet
} // namespace roc
