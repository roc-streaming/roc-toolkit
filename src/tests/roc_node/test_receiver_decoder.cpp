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
#include "roc_node/receiver_decoder.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace node {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;

void write_slot_metrics(const pipeline::ReceiverSlotMetrics& slot_metrics,
                        void* slot_arg) {
    *(pipeline::ReceiverSlotMetrics*)slot_arg = slot_metrics;
}

void write_party_metrics(const pipeline::ReceiverParticipantMetrics& party_metrics,
                         size_t party_index,
                         void* party_arg) {
    ((pipeline::ReceiverParticipantMetrics*)party_arg)[party_index] = party_metrics;
}

} // namespace

TEST_GROUP(receiver_decoder) {
    ContextConfig context_config;
    pipeline::ReceiverSourceConfig receiver_config;
};

TEST(receiver_decoder, source) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver_decoder.init_status());

    LONGS_EQUAL(receiver_decoder.source().sample_spec().sample_rate(),
                receiver_config.common.output_sample_spec.sample_rate());
}

TEST(receiver_decoder, write_packet) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver_decoder.init_status());

    uint8_t packet[MaxBufSize] = {};

    LONGS_EQUAL(status::StatusBadInterface,
                receiver_decoder.write_packet(address::Iface_AudioSource, packet,
                                              sizeof(packet)));
    LONGS_EQUAL(status::StatusBadInterface,
                receiver_decoder.write_packet(address::Iface_AudioRepair, packet,
                                              sizeof(packet)));
    LONGS_EQUAL(status::StatusBadInterface,
                receiver_decoder.write_packet(address::Iface_AudioControl, packet,
                                              sizeof(packet)));
}

TEST(receiver_decoder, read_packet) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver_decoder.init_status());

    uint8_t packet[MaxBufSize] = {};
    size_t packet_size = sizeof(packet);

    LONGS_EQUAL(
        status::StatusBadInterface,
        receiver_decoder.read_packet(address::Iface_AudioSource, packet, &packet_size));
    LONGS_EQUAL(
        status::StatusBadInterface,
        receiver_decoder.read_packet(address::Iface_AudioRepair, packet, &packet_size));
    LONGS_EQUAL(
        status::StatusBadInterface,
        receiver_decoder.read_packet(address::Iface_AudioControl, packet, &packet_size));
}

TEST(receiver_decoder, activate_no_fec) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver_decoder.init_status());

    CHECK(receiver_decoder.activate(address::Iface_AudioSource, address::Proto_RTP));
}

TEST(receiver_decoder, activate_fec) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver_decoder.init_status());

    if (fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        CHECK(receiver_decoder.activate(address::Iface_AudioSource,
                                        address::Proto_RTP_RS8M_Source));
        CHECK(receiver_decoder.activate(address::Iface_AudioRepair,
                                        address::Proto_RS8M_Repair));
    } else {
        CHECK(!receiver_decoder.activate(address::Iface_AudioSource,
                                         address::Proto_RTP_RS8M_Source));
        CHECK(!receiver_decoder.activate(address::Iface_AudioRepair,
                                         address::Proto_RS8M_Repair));
    }
}

TEST(receiver_decoder, metrics) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver_decoder.init_status());

    pipeline::ReceiverSlotMetrics slot_metrics;
    pipeline::ReceiverParticipantMetrics party_metrics;

    CHECK(receiver_decoder.get_metrics(write_slot_metrics, &slot_metrics,
                                       write_party_metrics, &party_metrics));

    LONGS_EQUAL(0, slot_metrics.num_participants);
}

} // namespace node
} // namespace roc
