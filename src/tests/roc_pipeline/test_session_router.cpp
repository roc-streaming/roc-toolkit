/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/receiver_session_router.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace pipeline {

namespace {

enum { MaxBufSize = 1000 };

core::HeapArena arena;

packet::PacketFactory packet_factory(arena, MaxBufSize);
audio::FrameFactory frame_factory(arena, MaxBufSize * sizeof(audio::sample_t));

audio::ProcessorMap processor_map(arena);
rtp::EncodingMap encoding_map(arena);

} // namespace

TEST_GROUP(session_router) {
    packet::stream_source_t ssrc1;
    packet::stream_source_t ssrc2;
    packet::stream_source_t ssrc3;

    address::SocketAddr addr1;
    address::SocketAddr addr2;

    const char* cname1;
    const char* cname2;

    core::SharedPtr<ReceiverSession> sess1;
    core::SharedPtr<ReceiverSession> sess2;

    void setup() {
        ssrc1 = 11;
        ssrc2 = 22;
        ssrc3 = 33;

        addr1 = test::new_address(11);
        addr2 = test::new_address(22);

        cname1 = "test_cname1";
        cname2 = "test_cname2";

        {
            ReceiverSessionConfig session_config;
            ReceiverCommonConfig common_config;

            sess1 = new (arena)
                ReceiverSession(session_config, common_config, processor_map,
                                encoding_map, packet_factory, frame_factory, arena, NULL);
            sess2 = new (arena)
                ReceiverSession(session_config, common_config, processor_map,
                                encoding_map, packet_factory, frame_factory, arena, NULL);
        }
    }
};

TEST(session_router, add_session_remove_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    router.remove_session(sess1);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));
}

TEST(session_router, two_sessions) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr2));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr2));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess2, ssrc2, addr2));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_address(addr2) == sess2);
}

TEST(session_router, add_session_link_ssrc) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_ssrc_add_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_main_add_session_link_extra) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_extra_add_session_link_main) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_two_extra_add_session_link_main) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc3, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_source(ssrc3));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_source(ssrc3));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_source(ssrc3) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, add_session_link_ssrc_remove_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    router.remove_session(sess1);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));
}

TEST(session_router, add_session_link_ssrc_unlink_main) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    router.unlink_source(ssrc1);

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, add_session_link_ssrc_unlink_extra) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    router.unlink_source(ssrc2);

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_ssrc_unlink_main_add_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    router.unlink_source(ssrc1);

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_ssrc_unlink_extra_add_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    router.unlink_source(ssrc2);

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, add_session_remove_session_link_ssrc_add_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));
    router.remove_session(sess1);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_twice_before_add_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_twice_after_add_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, link_twice_around_add_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, relink_main_ssrc_from_old_cname_to_existing_cname) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname2));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));
    LONGS_EQUAL(status::StatusOK, router.add_session(sess2, ssrc2, addr2));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_address(addr2) == sess2);

    // ssrc1 switches from cname1 to cname2
    // ssrc1 was used with add_session(), so its session also moves to cname2
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname2));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1); // updated
    CHECK(!router.find_by_address(addr2));        // updated
}

TEST(session_router, relink_extra_ssrc_from_old_cname_to_existing_cname) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname2));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc3, cname2));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));
    LONGS_EQUAL(status::StatusOK, router.add_session(sess2, ssrc2, addr2));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_source(ssrc3) == sess2);
    CHECK(router.find_by_address(addr2) == sess2);

    // ssrc3 switches from cname2 to cname1
    // ssrc3 was not used with add_session(), so cname1 keeps its session
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc3, cname1));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_source(ssrc3) == sess1); // updated
    CHECK(router.find_by_address(addr2) == sess2);
}

TEST(session_router, relink_main_ssrc_from_old_cname_to_nonexistent_cname) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc3, cname1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_source(ssrc3) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_address(addr2));

    // ssrc1 switches from cname1 to cname2 (which didn't exist yet)
    // ssrc1 was used with add_session(), so its session also moves to cname2
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname2));

    // ssrc2 and ssrc3 remain linked to cname1, without session
    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_source(ssrc3));
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_address(addr2));

    // link new session to ssrc3 (and so ssrc2 too)
    LONGS_EQUAL(status::StatusOK, router.add_session(sess2, ssrc3, addr2));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_source(ssrc3) == sess2);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_address(addr2) == sess2);
}

TEST(session_router, relink_extra_ssrc_from_old_cname_to_nonexistent_cname) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc3, cname1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_source(ssrc3) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_address(addr2));

    // ssrc2 and ssrc3 switch from cname1 to cname2 (which didn't exist yet)
    // ssrc2 and ssrc3 weren't used with add_session(), so cname1 keeps its session
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname2));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc3, cname2));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_source(ssrc3));
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_address(addr2));

    // link new session to ssrc3 (and so ssrc2 too)
    LONGS_EQUAL(status::StatusOK, router.add_session(sess2, ssrc3, addr2));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_source(ssrc3) == sess2);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_address(addr2) == sess2);
}

TEST(session_router, unlink_ssrc_without_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc3, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_source(ssrc3));
    CHECK(!router.find_by_address(addr1));

    router.unlink_source(ssrc3);

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_source(ssrc3));
    CHECK(!router.find_by_address(addr1));

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(!router.find_by_source(ssrc3));
    CHECK(router.find_by_address(addr1) == sess1);
}

TEST(session_router, unlink_ssrc_without_cname) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));
    LONGS_EQUAL(status::StatusOK, router.add_session(sess2, ssrc2, addr2));

    LONGS_EQUAL(2, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_address(addr2) == sess2);

    router.unlink_source(ssrc1);

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));
    CHECK(router.find_by_source(ssrc2) == sess2);
    CHECK(router.find_by_address(addr2) == sess2);
}

TEST(session_router, unlink_ssrc_with_session_and_cname) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    router.unlink_source(ssrc1);

    LONGS_EQUAL(1, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    router.unlink_source(ssrc2);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));
}

TEST(session_router, unlink_nonexistent_ssrc) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    // unlink nonexistent
    router.unlink_source(ssrc2);

    // nothing changes
    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    // unlink existing
    router.unlink_source(ssrc1);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));

    // unlink already unlinked
    router.unlink_source(ssrc1);

    // nothing changes
    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));
}

TEST(session_router, remove_session_with_linked_ssrcs) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc1, cname1));
    LONGS_EQUAL(status::StatusOK, router.link_source(ssrc2, cname1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_source(ssrc2) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    router.remove_session(sess1);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr1));
}

TEST(session_router, remove_nonexistent_session) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    // remove nonexistent
    router.remove_session(sess2);

    // nothing changes
    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);

    // remove existing
    router.remove_session(sess1);

    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));

    // remove already removed
    router.remove_session(sess1);

    // nothing changes
    LONGS_EQUAL(0, router.num_routes());
    CHECK(!router.find_by_source(ssrc1));
    CHECK(!router.find_by_address(addr1));
}

TEST(session_router, conflict_session_exists) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr2));

    LONGS_EQUAL(status::StatusNoRoute, router.add_session(sess1, ssrc2, addr2));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr2));
}

TEST(session_router, conflict_address_exists) {
    ReceiverSessionRouter router(arena);

    LONGS_EQUAL(status::StatusOK, router.add_session(sess1, ssrc1, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr2));

    LONGS_EQUAL(status::StatusNoRoute, router.add_session(sess2, ssrc2, addr1));

    LONGS_EQUAL(1, router.num_routes());
    CHECK(router.find_by_source(ssrc1) == sess1);
    CHECK(router.find_by_address(addr1) == sess1);
    CHECK(!router.find_by_source(ssrc2));
    CHECK(!router.find_by_address(addr2));
}

} // namespace pipeline
} // namespace roc
