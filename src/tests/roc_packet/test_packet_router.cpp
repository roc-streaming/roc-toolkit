/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/packet_queue.h"
#include "roc_packet/packet_router.h"

#include "test_packet.h"

namespace roc {
namespace test {

using namespace packet;

TEST_GROUP(packet_router){};

TEST(packet_router, no_routes) {
    PacketRouter router;
    IPacketConstPtr p = new_audio_packet(1);

    CHECK(!router.may_route(p));
    CHECK(!router.may_autodetect_route(p));

    router.write(p);

    LONGS_EQUAL(1, p->getref());
}

TEST(packet_router, one_route) {
    PacketRouter router;
    PacketQueue queue_a;

    router.add_route(queue_a, IPacket::HasAudio);

    IPacketConstPtr pa1 = new_audio_packet(1);
    IPacketConstPtr pa2 = new_audio_packet(2);

    IPacketConstPtr pf1 = new_fec_packet(1);
    IPacketConstPtr pf2 = new_fec_packet(2);

    CHECK(!router.may_route(pa1));
    CHECK(!router.may_route(pa2));
    CHECK(!router.may_route(pf1));
    CHECK(!router.may_route(pf2));

    CHECK(router.may_autodetect_route(pa1));
    CHECK(router.may_autodetect_route(pa2));
    CHECK(!router.may_autodetect_route(pf1));
    CHECK(!router.may_autodetect_route(pf2));

    router.write(pa1);
    CHECK(queue_a.read() == pa1);

    CHECK(router.may_route(pa1));
    CHECK(!router.may_route(pa2));
    CHECK(router.may_route(pf1));
    CHECK(!router.may_route(pf2));

    CHECK(!router.may_autodetect_route(pa1));
    CHECK(!router.may_autodetect_route(pa2));
    CHECK(!router.may_autodetect_route(pf1));
    CHECK(!router.may_autodetect_route(pf2));

    router.write(pa1);
    CHECK(queue_a.read() == pa1);

    router.write(pf1);
    CHECK(!queue_a.read());
}

TEST(packet_router, two_routes) {
    PacketRouter router;
    PacketQueue queue_a;
    PacketQueue queue_f;

    router.add_route(queue_a, IPacket::HasAudio);
    router.add_route(queue_f, IPacket::HasFEC);

    IPacketConstPtr pa1 = new_audio_packet(1);
    IPacketConstPtr pa2 = new_audio_packet(2);

    IPacketConstPtr pf1 = new_fec_packet(1);
    IPacketConstPtr pf2 = new_fec_packet(2);

    CHECK(!router.may_route(pa1));
    CHECK(!router.may_route(pa2));
    CHECK(!router.may_route(pf1));
    CHECK(!router.may_route(pf2));

    CHECK(router.may_autodetect_route(pa1));
    CHECK(router.may_autodetect_route(pa2));
    CHECK(router.may_autodetect_route(pf1));
    CHECK(router.may_autodetect_route(pf2));

    router.write(pa1);
    CHECK(queue_a.read() == pa1);

    router.write(pf2);
    CHECK(queue_f.read() == pf2);

    CHECK(router.may_route(pa1));
    CHECK(router.may_route(pa2));
    CHECK(router.may_route(pf1));
    CHECK(router.may_route(pf2));

    CHECK(!router.may_autodetect_route(pa1));
    CHECK(!router.may_autodetect_route(pa2));
    CHECK(!router.may_autodetect_route(pf1));
    CHECK(!router.may_autodetect_route(pf2));

    router.write(pa1);
    CHECK(queue_a.read() == pa1);

    router.write(pf2);
    CHECK(queue_f.read() == pf2);

    router.write(pa2);
    CHECK(!queue_a.read());

    router.write(pf1);
    CHECK(!queue_f.read());
}

} // namespace test
} // namespace roc
