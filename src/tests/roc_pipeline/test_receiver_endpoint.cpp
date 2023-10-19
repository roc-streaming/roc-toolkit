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

class StatusWriter : public packet::IWriter, public core::NonCopyable<> {
public:
    explicit StatusWriter(status::StatusCode code)
        : code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr&) {
        return code_;
    }

private:
    status::StatusCode code_;
};

void check_no_memory(bool is_valid,
                     const address::Protocol* protos,
                     unsigned protos_count,
                     packet::PacketFactory& packet_factory,
                     core::BufferFactory<uint8_t>& buffer_factory,
                     core::BufferFactory<audio::sample_t>& sample_factory) {
    test::NoopArena noop_arena;

    for (unsigned n = 0; n < protos_count; ++n) {
        rtp::FormatMap fmt_map(noop_arena);
        audio::Mixer mixer(sample_factory, false);

        ReceiverState receiver_state;
        ReceiverConfig receiver_config;
        ReceiverSessionGroup session_group(receiver_config, receiver_state, mixer,
                                           fmt_map, packet_factory, buffer_factory,
                                           sample_factory, noop_arena);

        ReceiverEndpoint endpoint(protos[n], receiver_state, session_group, fmt_map,
                                  noop_arena);

        CHECK(is_valid == endpoint.is_valid());
    }
}

enum { PacketSz = 512 };

const uint8_t Ref_rtp[] = {
    /* RTP header */
    0x80, 0x0B, 0x55, 0x66, //
    0x77, 0x88, 0x99, 0xaa, //
    0x11, 0x22, 0x33, 0x44, //
    /* Payload */
    0x01, 0x02, 0x03, 0x04, //
    0x05, 0x06, 0x07, 0x08, //
    0x09, 0x0a
};

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> buffer_factory(arena, PacketSz);
core::BufferFactory<audio::sample_t> sample_factory(arena, PacketSz);

} // namespace

TEST_GROUP(receiver_endpoint) {};

TEST(receiver_endpoint, valid) {
    rtp::FormatMap fmt_map(arena);
    audio::Mixer mixer(sample_factory, false);

    ReceiverState receiver_state;
    ReceiverConfig receiver_config;
    ReceiverSessionGroup session_group(receiver_config, receiver_state, mixer, fmt_map,
                                       packet_factory, buffer_factory, sample_factory,
                                       arena);

    ReceiverEndpoint endpoint(address::Proto_RTP, receiver_state, session_group, fmt_map,
                              arena);
    CHECK(endpoint.is_valid());
}

TEST(receiver_endpoint, is_valid_unknown_proto) {
    rtp::FormatMap fmt_map(arena);
    audio::Mixer mixer(sample_factory, false);

    ReceiverState receiver_state;
    ReceiverConfig receiver_config;
    ReceiverSessionGroup session_group(receiver_config, receiver_state, mixer, fmt_map,
                                       packet_factory, buffer_factory, sample_factory,
                                       arena);

    ReceiverEndpoint endpoint(address::Proto_None, receiver_state, session_group, fmt_map,
                              arena);
    CHECK(!endpoint.is_valid());
}

TEST(receiver_endpoint, is_valid_no_memory) {
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

    check_no_memory(false, protos_require_memory, ROC_ARRAY_SIZE(protos_require_memory),
                    packet_factory, buffer_factory, sample_factory);

    check_no_memory(true, protos_do_not_require_memory,
                    ROC_ARRAY_SIZE(protos_do_not_require_memory), packet_factory,
                    buffer_factory, sample_factory);
}

TEST(receiver_endpoint, write_read_packet) {
    rtp::FormatMap fmt_map(arena);
    audio::Mixer mixer(sample_factory, false);

    ReceiverState receiver_state;
    ReceiverConfig receiver_config;
    ReceiverSessionGroup session_group(receiver_config, receiver_state, mixer, fmt_map,
                                       packet_factory, buffer_factory, sample_factory,
                                       arena);

    ReceiverEndpoint endpoint(address::Proto_RTP, receiver_state, session_group, fmt_map,
                              arena);
    CHECK(endpoint.is_valid());
    CHECK(!receiver_state.has_pending_packets());

    packet::PacketPtr pp = packet_factory.new_packet();
    CHECK(pp);

    core::Slice<uint8_t> buffer = buffer_factory.new_buffer();
    CHECK(buffer);

    buffer.reslice(0, sizeof(Ref_rtp));
    for (size_t i = 0; i < sizeof(Ref_rtp); i++) {
        buffer.data()[i] = Ref_rtp[i];
    }

    pp->set_data(buffer);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, endpoint.writer().write(pp));
    CHECK(receiver_state.has_pending_packets());

    UNSIGNED_LONGS_EQUAL(status::StatusOK, endpoint.pull_packets());
    CHECK(!receiver_state.has_pending_packets());
}

TEST(receiver_endpoint, pull_packets_failed) {
    rtp::FormatMap fmt_map(arena);
    StatusWriter writer(status::StatusUnknown);
    ReceiverState receiver_state;

    ReceiverEndpoint endpoint(address::Proto_RTP, receiver_state, writer, fmt_map, arena);
    CHECK(endpoint.is_valid());
    CHECK(!receiver_state.has_pending_packets());

    packet::PacketPtr pp = packet_factory.new_packet();
    CHECK(pp);

    core::Slice<uint8_t> buffer = buffer_factory.new_buffer();
    CHECK(buffer);

    buffer.reslice(0, sizeof(Ref_rtp));
    for (size_t i = 0; i < sizeof(Ref_rtp); i++) {
        buffer.data()[i] = Ref_rtp[i];
    }

    pp->set_data(buffer);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, endpoint.writer().write(pp));
    CHECK(receiver_state.has_pending_packets());

    UNSIGNED_LONGS_EQUAL(status::StatusUnknown, endpoint.pull_packets());
    CHECK(!receiver_state.has_pending_packets());
}

} // namespace pipeline
} // namespace roc
