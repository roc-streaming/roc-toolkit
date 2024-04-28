/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/protocol.h"
#include "roc_core/heap_arena.h"
#include "roc_core/noop_arena.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_pipeline/sender_session.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_rtp/encoding_map.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

namespace {

enum { PacketSz = 512 };

core::HeapArena arena;

packet::PacketFactory packet_factory(arena, PacketSz);
audio::FrameFactory frame_factory(arena, PacketSz * sizeof(audio::sample_t));

audio::ProcessorMap processor_map(arena);
rtp::EncodingMap encoding_map(arena);

} // namespace

TEST_GROUP(sender_endpoint) {};

TEST(sender_endpoint, valid) {
    address::SocketAddr addr;
    packet::FifoQueue queue;

    SenderSinkConfig sink_config;
    StateTracker state_tracker;
    SenderSession session(sink_config, processor_map, encoding_map, packet_factory,
                          frame_factory, arena, NULL);

    SenderEndpoint endpoint(address::Proto_RTP, state_tracker, session, addr, queue,
                            arena);
    LONGS_EQUAL(status::StatusOK, endpoint.init_status());
}

TEST(sender_endpoint, invalid_proto) {
    address::SocketAddr addr;
    packet::FifoQueue queue;
    core::HeapArena arena;

    SenderSinkConfig sink_config;
    StateTracker state_tracker;
    SenderSession session(sink_config, processor_map, encoding_map, packet_factory,
                          frame_factory, arena, NULL);

    SenderEndpoint endpoint(address::Proto_None, state_tracker, session, addr, queue,
                            arena);
    LONGS_EQUAL(status::StatusBadProtocol, endpoint.init_status());
}

TEST(sender_endpoint, no_memory) {
    const address::Protocol protos[] = {
        address::Proto_RTP_LDPC_Source,
        address::Proto_RTP_RS8M_Source,
        address::Proto_RS8M_Repair,
        address::Proto_LDPC_Repair,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(protos); ++n) {
        address::SocketAddr addr;
        packet::FifoQueue queue;

        SenderSinkConfig sink_config;
        StateTracker state_tracker;
        SenderSession session(sink_config, processor_map, encoding_map, packet_factory,
                              frame_factory, arena, NULL);

        SenderEndpoint endpoint(protos[n], state_tracker, session, addr, queue,
                                core::NoopArena);
        LONGS_EQUAL(status::StatusNoMem, endpoint.init_status());
    }
}

} // namespace pipeline
} // namespace roc
