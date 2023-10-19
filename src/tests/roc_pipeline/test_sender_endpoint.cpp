/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/noop_arena.h"

#include "roc_address/protocol.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

namespace {

void check_no_memory(bool is_valid,
                     const address::Protocol* protos,
                     unsigned protos_count) {
    test::NoopArena noop_arena;

    for (unsigned n = 0; n < protos_count; ++n) {
        address::SocketAddr addr;
        packet::Queue queue;

        SenderEndpoint endpoint(protos[n], addr, queue, noop_arena);
        CHECK(is_valid == endpoint.is_valid());
    }
}

} // namespace

TEST_GROUP(sender_endpoint) {};

TEST(sender_endpoint, valid) {
    address::SocketAddr addr;
    packet::Queue queue;
    core::HeapArena arena;

    SenderEndpoint endpoint(address::Proto_RTP, addr, queue, arena);
    CHECK(endpoint.is_valid());
}

TEST(sender_endpoint, is_valid_unknown_proto) {
    address::SocketAddr addr;
    packet::Queue queue;
    core::HeapArena arena;

    SenderEndpoint endpoint(address::Proto_None, addr, queue, arena);
    CHECK(!endpoint.is_valid());
}

TEST(sender_endpoint, is_valid_no_memory) {
    const address::Protocol protos_require_memory[] = {
        address::Proto_RTP_LDPC_Source,
        address::Proto_RTP_RS8M_Source,
        address::Proto_RS8M_Repair,
        address::Proto_LDPC_Repair,
    };

    const address::Protocol protos_do_not_require_memory[] = {
        address::Proto_RTP,
        address::Proto_RTCP,
    };

    check_no_memory(false, protos_require_memory, ROC_ARRAY_SIZE(protos_require_memory));

    check_no_memory(true, protos_do_not_require_memory,
                    ROC_ARRAY_SIZE(protos_do_not_require_memory));
}

TEST(sender_endpoint, write_read_packet) {
    address::SocketAddr addr;
    packet::Queue queue;
    core::HeapArena arena;

    SenderEndpoint endpoint(address::Proto_RTP, addr, queue, arena);
    CHECK(endpoint.is_valid());

    packet::PacketFactory packet_factory(arena);
    packet::PacketPtr wp = packet_factory.new_packet();
    CHECK(wp);
    wp->add_flags(packet::Packet::FlagPrepared | packet::Packet::FlagComposed);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, endpoint.writer().write(wp));

    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rp));
    CHECK(wp == rp);
}

} // namespace pipeline
} // namespace roc
