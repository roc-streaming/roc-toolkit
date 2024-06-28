/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/heap_arena.h"
#include "roc_fec/codec_map.h"
#include "roc_node/context.h"
#include "roc_node/sender_encoder.h"
#include "roc_status/status_code.h"

namespace roc {
namespace node {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;

void write_slot_metrics(const pipeline::SenderSlotMetrics& slot_metrics, void* slot_arg) {
    *(pipeline::SenderSlotMetrics*)slot_arg = slot_metrics;
}

void write_party_metrics(const pipeline::SenderParticipantMetrics& party_metrics,
                         size_t party_index,
                         void* party_arg) {
    ((pipeline::SenderParticipantMetrics*)party_arg)[party_index] = party_metrics;
}

} // namespace

TEST_GROUP(sender_encoder) {
    ContextConfig context_config;
    pipeline::SenderSinkConfig sender_config;
};

TEST(sender_encoder, sink) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    SenderEncoder sender_encoder(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender_encoder.init_status());

    LONGS_EQUAL(sender_encoder.sink().sample_spec().sample_rate(),
                sender_config.input_sample_spec.sample_rate());
}

TEST(sender_encoder, read_packet) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    SenderEncoder sender_encoder(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender_encoder.init_status());

    packet::PacketPtr pp;

    uint8_t packet[MaxBufSize] = {};
    size_t packet_size = sizeof(packet);

    LONGS_EQUAL(
        status::StatusBadInterface,
        sender_encoder.read_packet(address::Iface_AudioSource, packet, &packet_size));
    LONGS_EQUAL(
        status::StatusBadInterface,
        sender_encoder.read_packet(address::Iface_AudioRepair, packet, &packet_size));
    LONGS_EQUAL(
        status::StatusBadInterface,
        sender_encoder.read_packet(address::Iface_AudioControl, packet, &packet_size));
}

TEST(sender_encoder, write_packet) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    SenderEncoder sender_encoder(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender_encoder.init_status());

    uint8_t packet[MaxBufSize] = {};

    LONGS_EQUAL(
        status::StatusBadInterface,
        sender_encoder.write_packet(address::Iface_AudioSource, packet, sizeof(packet)));
    LONGS_EQUAL(
        status::StatusBadInterface,
        sender_encoder.write_packet(address::Iface_AudioRepair, packet, sizeof(packet)));
    LONGS_EQUAL(
        status::StatusBadInterface,
        sender_encoder.write_packet(address::Iface_AudioControl, packet, sizeof(packet)));
}

TEST(sender_encoder, activate_no_fec) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    sender_config.fec_encoder.scheme = packet::FEC_None;

    SenderEncoder sender_encoder(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender_encoder.init_status());
    CHECK(!sender_encoder.is_complete());

    CHECK(sender_encoder.activate(address::Iface_AudioSource, address::Proto_RTP));
    CHECK(sender_encoder.is_complete());
}

TEST(sender_encoder, activate_fec) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    SenderEncoder sender_encoder(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender_encoder.init_status());
    CHECK(!sender_encoder.is_complete());

    if (fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        CHECK(sender_encoder.activate(address::Iface_AudioSource,
                                      address::Proto_RTP_RS8M_Source));
        CHECK(!sender_encoder.is_complete());

        CHECK(sender_encoder.activate(address::Iface_AudioRepair,
                                      address::Proto_RS8M_Repair));
        CHECK(sender_encoder.is_complete());
    } else {
        CHECK(!sender_encoder.activate(address::Iface_AudioSource,
                                       address::Proto_RTP_RS8M_Source));
        CHECK(!sender_encoder.is_complete());

        CHECK(!sender_encoder.activate(address::Iface_AudioRepair,
                                       address::Proto_RS8M_Repair));
        CHECK(!sender_encoder.is_complete());
    }
}

TEST(sender_encoder, metrics) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    SenderEncoder sender_encoder(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender_encoder.init_status());

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderParticipantMetrics party_metrics;

    CHECK(sender_encoder.get_metrics(write_slot_metrics, &slot_metrics,
                                     write_party_metrics, &party_metrics));

    LONGS_EQUAL(0, slot_metrics.num_participants);
}

} // namespace node
} // namespace roc
