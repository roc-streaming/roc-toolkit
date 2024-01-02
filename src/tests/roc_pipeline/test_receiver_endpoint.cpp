/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/protocol.h"
#include "roc_audio/mixer.h"
#include "roc_audio/sample.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_session_group.h"
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
};

enum { PacketSz = 512 };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, PacketSz);
core::BufferFactory<audio::sample_t> sample_buffer_factory(arena, PacketSz);
rtp::EncodingMap encoding_map(arena);

} // namespace

TEST_GROUP(receiver_endpoint) {};

TEST(receiver_endpoint, valid) {
    audio::Mixer mixer(sample_buffer_factory, false);

    StateTracker state_tracker;
    ReceiverConfig receiver_config;
    ReceiverSessionGroup session_group(receiver_config, state_tracker, mixer,
                                       encoding_map, packet_factory, byte_buffer_factory,
                                       sample_buffer_factory, arena);

    ReceiverEndpoint endpoint(address::Proto_RTP, state_tracker, session_group,
                              encoding_map, NULL, NULL, arena);
    CHECK(endpoint.is_valid());
}

TEST(receiver_endpoint, invalid_proto) {
    audio::Mixer mixer(sample_buffer_factory, false);

    StateTracker state_tracker;
    ReceiverConfig receiver_config;
    ReceiverSessionGroup session_group(receiver_config, state_tracker, mixer,
                                       encoding_map, packet_factory, byte_buffer_factory,
                                       sample_buffer_factory, arena);

    ReceiverEndpoint endpoint(address::Proto_None, state_tracker, session_group,
                              encoding_map, NULL, NULL, arena);
    CHECK(!endpoint.is_valid());
}

TEST(receiver_endpoint, no_memory) {
    const address::Protocol protos[] = {
        address::Proto_RTP_LDPC_Source,
        address::Proto_RTP_RS8M_Source,
        address::Proto_RS8M_Repair,
        address::Proto_LDPC_Repair,
    };

    NoMemArena nomem_arena;

    for (size_t n = 0; n < ROC_ARRAY_SIZE(protos); ++n) {
        audio::Mixer mixer(sample_buffer_factory, false);

        StateTracker state_tracker;
        ReceiverConfig receiver_config;
        ReceiverSessionGroup session_group(
            receiver_config, state_tracker, mixer, encoding_map, packet_factory,
            byte_buffer_factory, sample_buffer_factory, nomem_arena);

        ReceiverEndpoint endpoint(protos[n], state_tracker, session_group, encoding_map,
                                  NULL, NULL, nomem_arena);

        CHECK(!endpoint.is_valid());
    }
}

} // namespace pipeline
} // namespace roc
