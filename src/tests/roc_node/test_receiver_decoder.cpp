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

void handle_sess_metrics(const pipeline::ReceiverSessionMetrics& sess_metrics,
                         size_t sess_index,
                         void* sess_arg) {
    ((pipeline::ReceiverSessionMetrics*)sess_arg)[sess_index] = sess_metrics;
}

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

    CHECK_EQUAL(receiver_decoder.source().sample_spec().sample_rate(),
                receiver_config.common.output_sample_spec.sample_rate());
}

TEST(receiver_decoder, write) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    ReceiverDecoder receiver_decoder(context, receiver_config);
    CHECK(receiver_decoder.is_valid());

    packet::PacketPtr pp = packet_factory.new_packet();

    CHECK_EQUAL(status::StatusUnknown,
                receiver_decoder.write(address::Iface_AudioSource, pp));
    CHECK_EQUAL(status::StatusUnknown,
                receiver_decoder.write(address::Iface_AudioRepair, pp));
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
    pipeline::ReceiverSessionMetrics sess_metrics[10];
    size_t sess_metrics_size = 0;

    sess_metrics_size = ROC_ARRAY_SIZE(sess_metrics);
    CHECK(receiver_decoder.get_metrics(slot_metrics, handle_sess_metrics,
                                       &sess_metrics_size, sess_metrics));

    CHECK_EQUAL(0, slot_metrics.num_sessions);
    CHECK_EQUAL(0, sess_metrics_size);

    CHECK(receiver_decoder.activate(address::Iface_AudioSource, address::Proto_RTP));

    sess_metrics_size = ROC_ARRAY_SIZE(sess_metrics);
    CHECK(receiver_decoder.get_metrics(slot_metrics, handle_sess_metrics,
                                       &sess_metrics_size, sess_metrics));

    CHECK_EQUAL(0, slot_metrics.num_sessions);
    CHECK_EQUAL(0, sess_metrics_size);
}

} // namespace node
} // namespace roc
