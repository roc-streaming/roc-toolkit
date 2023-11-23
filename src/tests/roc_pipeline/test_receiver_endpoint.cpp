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
#include "roc_pipeline/receiver_state.h"
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
core::BufferFactory<uint8_t> buffer_factory(arena, PacketSz);
core::BufferFactory<audio::sample_t> sample_factory(arena, PacketSz);

} // namespace

TEST_GROUP(receiver_endpoint) {};

TEST(receiver_endpoint, valid) {
    rtp::EncodingMap encoding_map(arena);
    audio::Mixer mixer(sample_factory, false);

    ReceiverState receiver_state;
    ReceiverConfig receiver_config;
    ReceiverSessionGroup session_group(receiver_config, receiver_state, mixer,
                                       encoding_map, packet_factory, buffer_factory,
                                       sample_factory, arena);

    ReceiverEndpoint endpoint(address::Proto_RTP, receiver_state, session_group,
                              encoding_map, arena);
    CHECK(endpoint.is_valid());
}

TEST(receiver_endpoint, invalid_proto) {
    rtp::EncodingMap encoding_map(arena);
    audio::Mixer mixer(sample_factory, false);

    ReceiverState receiver_state;
    ReceiverConfig receiver_config;
    ReceiverSessionGroup session_group(receiver_config, receiver_state, mixer,
                                       encoding_map, packet_factory, buffer_factory,
                                       sample_factory, arena);

    ReceiverEndpoint endpoint(address::Proto_None, receiver_state, session_group,
                              encoding_map, arena);
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
        rtp::EncodingMap encoding_map(nomem_arena);
        audio::Mixer mixer(sample_factory, false);

        ReceiverState receiver_state;
        ReceiverConfig receiver_config;
        ReceiverSessionGroup session_group(receiver_config, receiver_state, mixer,
                                           encoding_map, packet_factory, buffer_factory,
                                           sample_factory, nomem_arena);

        ReceiverEndpoint endpoint(protos[n], receiver_state, session_group, encoding_map,
                                  nomem_arena);

        CHECK(!endpoint.is_valid());
    }
}

} // namespace pipeline
} // namespace roc
