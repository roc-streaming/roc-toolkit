/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_harness.h"
#include "test_helpers/control_writer.h"

#include "roc_address/protocol.h"
#include "roc_core/heap_arena.h"
#include "roc_core/noop_arena.h"
#include "roc_core/time.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_pipeline/sender_session.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_rtp/encoding_map.h"
#include "roc_sndio/device_defs.h"
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

// Write control packets with too short payload to the writer.
// When such packets are pulled into pipeline, they're just dropped.
void write_truncated_packets(packet::IWriter& writer, size_t count) {
    for (size_t n = 0; n < count; ++n) {
        packet::PacketPtr pp = packet_factory.new_packet();
        CHECK(pp);

        pp->add_flags(packet::Packet::FlagUDP);

        core::Slice<uint8_t> buff = packet_factory.new_packet_buffer();
        CHECK(buff);

        buff.reslice(0, 0);
        pp->set_buffer(buff);

        LONGS_EQUAL(status::StatusOK, writer.write(pp));
    }
}

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

// If endpoint is destroyed while there are packets that were written to it, but
// not pulled yet, it should unregister them from state tracker.
TEST(sender_endpoint, destroy_with_pending_packets) {
    address::SocketAddr addr;
    packet::FifoQueue queue;

    SenderSinkConfig sink_config;
    StateTracker state_tracker;
    SenderSession session(sink_config, processor_map, encoding_map, packet_factory,
                          frame_factory, arena, NULL);

    LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());

    {
        SenderEndpoint endpoint(address::Proto_RTCP, state_tracker, session, addr, queue,
                                arena);
        LONGS_EQUAL(status::StatusOK, endpoint.init_status());

        packet::IWriter* inbound_writer = endpoint.inbound_writer();
        CHECK(inbound_writer);

        test::ControlWriter control_writer(*inbound_writer, packet_factory,
                                           test::new_address(1111),
                                           test::new_address(2222));

        control_writer.write_sender_report(0, 0);
        control_writer.write_sender_report(0, 0);
        control_writer.write_sender_report(0, 0);

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
TEST(sender_endpoint, destroy_after_pull) {
    address::SocketAddr addr;
    packet::FifoQueue queue;

    SenderSinkConfig sink_config;
    StateTracker state_tracker;
    SenderSession session(sink_config, processor_map, encoding_map, packet_factory,
                          frame_factory, arena, NULL);

    {
        SenderEndpoint endpoint(address::Proto_RTCP, state_tracker, session, addr, queue,
                                arena);
        LONGS_EQUAL(status::StatusOK, endpoint.init_status());

        LONGS_EQUAL(status::StatusOK, session.create_control_pipeline(&endpoint));

        packet::IWriter* inbound_writer = endpoint.inbound_writer();
        CHECK(inbound_writer);

        write_truncated_packets(*inbound_writer, 3);

        LONGS_EQUAL(sndio::DeviceState_Active, state_tracker.get_state());

        LONGS_EQUAL(status::StatusOK,
                    endpoint.pull_packets(core::timestamp(core::ClockMonotonic)));

        LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());
    }

    LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());
}

// Pulled and pending packets are mixed: only pending ones should be unregistered
// when endpoint is destroyed.
TEST(sender_endpoint, destroy_after_pull_and_write) {
    address::SocketAddr addr;
    packet::FifoQueue queue;

    SenderSinkConfig sink_config;
    StateTracker state_tracker;
    SenderSession session(sink_config, processor_map, encoding_map, packet_factory,
                          frame_factory, arena, NULL);

    {
        SenderEndpoint endpoint(address::Proto_RTCP, state_tracker, session, addr, queue,
                                arena);
        LONGS_EQUAL(status::StatusOK, endpoint.init_status());

        LONGS_EQUAL(status::StatusOK, session.create_control_pipeline(&endpoint));

        packet::IWriter* inbound_writer = endpoint.inbound_writer();
        CHECK(inbound_writer);

        write_truncated_packets(*inbound_writer, 2);

        LONGS_EQUAL(status::StatusOK,
                    endpoint.pull_packets(core::timestamp(core::ClockMonotonic)));

        LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());

        write_truncated_packets(*inbound_writer, 3);

        LONGS_EQUAL(sndio::DeviceState_Active, state_tracker.get_state());
    }

    LONGS_EQUAL(sndio::DeviceState_Idle, state_tracker.get_state());
}

} // namespace pipeline
} // namespace roc
