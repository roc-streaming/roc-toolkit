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

namespace roc {
namespace node {

namespace {

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);

} // namespace

TEST_GROUP(receiver_decoder) {
    ContextConfig context_config;
    pipeline::ReceiverConfig receiver_config;
};

TEST(receiver_decoder, source) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    CHECK(receiver_decoder.is_valid());

    UNSIGNED_LONGS_EQUAL(receiver_decoder.source().sample_spec().sample_rate(),
                         receiver_config.common.output_sample_spec.sample_rate());
}

TEST(receiver_decoder, write) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    CHECK(receiver_decoder.is_valid());

    packet::PacketPtr pp = packet_factory.new_packet();

    CHECK(!receiver_decoder.write(address::Iface_AudioSource, pp));
    CHECK(!receiver_decoder.write(address::Iface_AudioRepair, pp));
}

TEST(receiver_decoder, activate_no_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    CHECK(receiver_decoder.is_valid());

    CHECK(receiver_decoder.activate(address::Iface_AudioSource, address::Proto_RTP));
}

TEST(receiver_decoder, activate_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    CHECK(receiver_decoder.is_valid());

    if (fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
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
    CHECK(context.is_valid());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    CHECK(receiver_decoder.is_valid());

    pipeline::ReceiverSlotMetrics slot_metrics;
    pipeline::ReceiverSessionMetrics sess_metrics;

    CHECK(receiver_decoder.get_metrics(slot_metrics, sess_metrics));
    UNSIGNED_LONGS_EQUAL(0, slot_metrics.num_sessions);

    CHECK(receiver_decoder.activate(address::Iface_AudioSource, address::Proto_RTP));

    CHECK(receiver_decoder.get_metrics(slot_metrics, sess_metrics));
    UNSIGNED_LONGS_EQUAL(0, slot_metrics.num_sessions);
}

} // namespace node
} // namespace roc
