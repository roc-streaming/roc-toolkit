/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_harness.h"
#include "test_helpers/packet_writer.h"

#include "roc_address/protocol.h"
#include "roc_audio/mixer.h"
#include "roc_audio/sample.h"
#include "roc_core/heap_arena.h"
#include "roc_core/noop_arena.h"
#include "roc_core/time.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_session_group.h"
#include "roc_rtp/headers.h"
#include "roc_sndio/device_defs.h"

namespace roc {
namespace pipeline {

namespace {

enum { PacketSz = 512, SamplesPerPacket = 100 };

const rtp::PayloadType PayloadType_Ch2 = rtp::PayloadType_L16_Stereo;

core::HeapArena arena;

packet::PacketFactory packet_factory(arena, PacketSz);
audio::FrameFactory frame_factory(arena, PacketSz * sizeof(audio::sample_t));

audio::ProcessorMap processor_map(arena);
rtp::EncodingMap encoding_map(arena);

} // namespace

TEST_GROUP(receiver_endpoint) {};

TEST(receiver_endpoint, valid) {
    audio::Mixer mixer(DefaultSampleSpec, false, frame_factory, arena);

    StateTracker state_tracker;
    ReceiverSourceConfig source_config;
    ReceiverSlotConfig slot_config;
    ReceiverSessionGroup session_group(source_config, slot_config, state_tracker, mixer,
                                       processor_map, encoding_map, packet_factory,
                                       frame_factory, arena, NULL);

    ReceiverEndpoint endpoint(address::Proto_RTP, state_tracker, session_group,
                              encoding_map, address::SocketAddr(), NULL, arena);
    LONGS_EQUAL(status::StatusOK, endpoint.init_status());
}

TEST(receiver_endpoint, invalid_proto) {
    audio::Mixer mixer(DefaultSampleSpec, false, frame_factory, arena);

    StateTracker state_tracker;
    ReceiverSourceConfig source_config;
    ReceiverSlotConfig slot_config;
    ReceiverSessionGroup session_group(source_config, slot_config, state_tracker, mixer,
                                       processor_map, encoding_map, packet_factory,
                                       frame_factory, arena, NULL);

    ReceiverEndpoint endpoint(address::Proto_None, state_tracker, session_group,
                              encoding_map, address::SocketAddr(), NULL, arena);
    LONGS_EQUAL(status::StatusBadProtocol, endpoint.init_status());
}

TEST(receiver_endpoint, no_memory) {
    const address::Protocol protos[] = {
        address::Proto_RTP_LDPC_Source,
        address::Proto_RTP_RS8M_Source,
        address::Proto_RS8M_Repair,
        address::Proto_LDPC_Repair,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(protos); ++n) {
        audio::Mixer mixer(DefaultSampleSpec, false, frame_factory, arena);

        StateTracker state_tracker;
        ReceiverSourceConfig source_config;
        ReceiverSlotConfig slot_config;
        ReceiverSessionGroup session_group(
            source_config, slot_config, state_tracker, mixer, processor_map, encoding_map,
            packet_factory, frame_factory, core::NoopArena, NULL);

        ReceiverEndpoint endpoint(protos[n], state_tracker, session_group, encoding_map,
                                  address::SocketAddr(), NULL, core::NoopArena);
        LONGS_EQUAL(status::StatusNoMem, endpoint.init_status());
    }
}

// If endpoint is destroyed while there are packets that were written to it, but
// not pulled yet, it should unregister them from state tracker.
TEST(receiver_endpoint, destroy_with_pending_packets) {
    audio::Mixer mixer(DefaultSampleSpec, false, frame_factory, arena);

    StateTracker state_tracker;
    ReceiverSourceConfig source_config;
    ReceiverSlotConfig slot_config;
    ReceiverSessionGroup session_group(source_config, slot_config, state_tracker, mixer,
                                       processor_map, encoding_map, packet_factory,
                                       frame_factory, arena, NULL);

    LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());

    {
        ReceiverEndpoint endpoint(address::Proto_RTP, state_tracker, session_group,
                                  encoding_map, address::SocketAddr(), NULL, arena);
        LONGS_EQUAL(status::StatusOK, endpoint.init_status());

        test::PacketWriter packet_writer(arena, endpoint.inbound_writer(), encoding_map,
                                         packet_factory, 11, test::new_address(1111),
                                         test::new_address(2222), PayloadType_Ch2);

        packet_writer.write_packets(3, SamplesPerPacket, DefaultSampleSpec);

        // Packets are not pulled, so they're still pending.
        LONGS_EQUAL(sndio::DeviceState_Active, state_tracker.get_state());
    }

    LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());

    unsigned ret_states = 0;
    CHECK(state_tracker.wait_state(sndio::DeviceState_Idle, ret_states,
                                   core::timestamp(core::ClockMonotonic) + core::Second));
    LONGS_EQUAL(sndio::DeviceState_Idle, ret_states);
}

// Packets that were pulled are already unregistered, and endpoint should not
// unregister them again when it's destroyed (state tracker panics on that).
// Packets are corrupted, so that pulling them doesn't create sessions.
TEST(receiver_endpoint, destroy_after_pull) {
    audio::Mixer mixer(DefaultSampleSpec, false, frame_factory, arena);

    StateTracker state_tracker;
    ReceiverSourceConfig source_config;
    ReceiverSlotConfig slot_config;
    ReceiverSessionGroup session_group(source_config, slot_config, state_tracker, mixer,
                                       processor_map, encoding_map, packet_factory,
                                       frame_factory, arena, NULL);

    {
        ReceiverEndpoint endpoint(address::Proto_RTP, state_tracker, session_group,
                                  encoding_map, address::SocketAddr(), NULL, arena);
        LONGS_EQUAL(status::StatusOK, endpoint.init_status());

        test::PacketWriter packet_writer(arena, endpoint.inbound_writer(), encoding_map,
                                         packet_factory, 11, test::new_address(1111),
                                         test::new_address(2222), PayloadType_Ch2);

        packet_writer.corrupt_packets(true);
        packet_writer.write_packets(3, SamplesPerPacket, DefaultSampleSpec);

        LONGS_EQUAL(sndio::DeviceState_Active, state_tracker.get_state());

        LONGS_EQUAL(status::StatusOK,
                    endpoint.pull_packets(core::timestamp(core::ClockMonotonic)));

        LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());
    }

    LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());
}

// Pulled and pending packets are mixed: only pending ones should be unregistered
// when endpoint is destroyed.
TEST(receiver_endpoint, destroy_after_pull_and_write) {
    audio::Mixer mixer(DefaultSampleSpec, false, frame_factory, arena);

    StateTracker state_tracker;
    ReceiverSourceConfig source_config;
    ReceiverSlotConfig slot_config;
    ReceiverSessionGroup session_group(source_config, slot_config, state_tracker, mixer,
                                       processor_map, encoding_map, packet_factory,
                                       frame_factory, arena, NULL);

    {
        ReceiverEndpoint endpoint(address::Proto_RTP, state_tracker, session_group,
                                  encoding_map, address::SocketAddr(), NULL, arena);
        LONGS_EQUAL(status::StatusOK, endpoint.init_status());

        test::PacketWriter packet_writer(arena, endpoint.inbound_writer(), encoding_map,
                                         packet_factory, 11, test::new_address(1111),
                                         test::new_address(2222), PayloadType_Ch2);

        packet_writer.corrupt_packets(true);
        packet_writer.write_packets(2, SamplesPerPacket, DefaultSampleSpec);

        LONGS_EQUAL(status::StatusOK,
                    endpoint.pull_packets(core::timestamp(core::ClockMonotonic)));

        LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());

        packet_writer.corrupt_packets(false);
        packet_writer.write_packets(3, SamplesPerPacket, DefaultSampleSpec);

        LONGS_EQUAL(sndio::DeviceState_Active, state_tracker.get_state());
    }

    LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());
}

} // namespace pipeline
} // namespace roc
