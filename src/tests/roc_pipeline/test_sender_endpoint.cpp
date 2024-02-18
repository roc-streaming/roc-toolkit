/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/protocol.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_pipeline/sender_session.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_rtp/encoding_map.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

namespace {

struct NoMemArena : public core::IArena, public core::NonCopyable<> {
    virtual void* allocate(size_t) {
        return NULL;
    }

    virtual void deallocate(void*) {
    }

    virtual size_t compute_allocated_size(size_t) const {
        return 0;
    }

    virtual size_t allocated_size(void*) const {
        return 0;
    }
};

enum { PacketSz = 512 };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, PacketSz);
core::BufferFactory<audio::sample_t> sample_buffer_factory(arena, PacketSz);
rtp::EncodingMap encoding_map(arena);

} // namespace

TEST_GROUP(sender_endpoint) {};

TEST(sender_endpoint, valid) {
    address::SocketAddr addr;
    packet::Queue queue;

    SenderSinkConfig sink_config;
    StateTracker state_tracker;
    SenderSession session(sink_config, encoding_map, packet_factory, byte_buffer_factory,
                          sample_buffer_factory, arena);

    SenderEndpoint endpoint(address::Proto_RTP, state_tracker, session, addr, queue,
                            arena);
    CHECK(endpoint.is_valid());
}

TEST(sender_endpoint, invalid_proto) {
    address::SocketAddr addr;
    packet::Queue queue;
    core::HeapArena arena;

    SenderSinkConfig sink_config;
    StateTracker state_tracker;
    SenderSession session(sink_config, encoding_map, packet_factory, byte_buffer_factory,
                          sample_buffer_factory, arena);

    SenderEndpoint endpoint(address::Proto_None, state_tracker, session, addr, queue,
                            arena);
    CHECK(!endpoint.is_valid());
}

TEST(sender_endpoint, no_memory) {
    const address::Protocol protos[] = {
        address::Proto_RTP_LDPC_Source,
        address::Proto_RTP_RS8M_Source,
        address::Proto_RS8M_Repair,
        address::Proto_LDPC_Repair,
    };

    NoMemArena nomem_arena;

    for (size_t n = 0; n < ROC_ARRAY_SIZE(protos); ++n) {
        address::SocketAddr addr;
        packet::Queue queue;

        SenderSinkConfig sink_config;
        StateTracker state_tracker;
        SenderSession session(sink_config, encoding_map, packet_factory,
                              byte_buffer_factory, sample_buffer_factory, arena);

        SenderEndpoint endpoint(protos[n], state_tracker, session, addr, queue,
                                nomem_arena);
        CHECK(!endpoint.is_valid());
    }
}

} // namespace pipeline
} // namespace roc
