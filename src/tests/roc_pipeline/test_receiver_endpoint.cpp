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
#include "roc_core/noop_arena.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_session_group.h"

namespace roc {
namespace pipeline {

namespace {

enum { PacketSz = 512 };

core::HeapArena arena;

packet::PacketFactory packet_factory(arena, PacketSz);
audio::FrameFactory frame_factory(arena, PacketSz * sizeof(audio::sample_t));

rtp::EncodingMap encoding_map(arena);

} // namespace

TEST_GROUP(receiver_endpoint) {};

TEST(receiver_endpoint, valid) {
    audio::Mixer mixer(frame_factory, DefaultSampleSpec, false);

    StateTracker state_tracker;
    ReceiverSourceConfig source_config;
    ReceiverSlotConfig slot_config;
    ReceiverSessionGroup session_group(source_config, slot_config, state_tracker, mixer,
                                       encoding_map, packet_factory, frame_factory,
                                       arena);

    ReceiverEndpoint endpoint(address::Proto_RTP, state_tracker, session_group,
                              encoding_map, address::SocketAddr(), NULL, arena);
    CHECK(endpoint.is_valid());
}

TEST(receiver_endpoint, invalid_proto) {
    audio::Mixer mixer(frame_factory, DefaultSampleSpec, false);

    StateTracker state_tracker;
    ReceiverSourceConfig source_config;
    ReceiverSlotConfig slot_config;
    ReceiverSessionGroup session_group(source_config, slot_config, state_tracker, mixer,
                                       encoding_map, packet_factory, frame_factory,
                                       arena);

    ReceiverEndpoint endpoint(address::Proto_None, state_tracker, session_group,
                              encoding_map, address::SocketAddr(), NULL, arena);
    CHECK(!endpoint.is_valid());
}

TEST(receiver_endpoint, no_memory) {
    const address::Protocol protos[] = {
        address::Proto_RTP_LDPC_Source,
        address::Proto_RTP_RS8M_Source,
        address::Proto_RS8M_Repair,
        address::Proto_LDPC_Repair,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(protos); ++n) {
        audio::Mixer mixer(frame_factory, DefaultSampleSpec, false);

        StateTracker state_tracker;
        ReceiverSourceConfig source_config;
        ReceiverSlotConfig slot_config;
        ReceiverSessionGroup session_group(source_config, slot_config, state_tracker,
                                           mixer, encoding_map, packet_factory,
                                           frame_factory, core::NoopArena);

        ReceiverEndpoint endpoint(protos[n], state_tracker, session_group, encoding_map,
                                  address::SocketAddr(), NULL, core::NoopArena);

        CHECK(!endpoint.is_valid());
    }
}

} // namespace pipeline
} // namespace roc
