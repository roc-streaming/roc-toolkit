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

core::HeapArena arena;

} // namespace

TEST_GROUP(sender_encoder) {
    ContextConfig context_config;
    pipeline::SenderConfig sender_config;
};

TEST(sender_encoder, sink) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    SenderEncoder sender_encoder(context, sender_config);
    CHECK(sender_encoder.is_valid());

    CHECK_EQUAL(sender_encoder.sink().sample_spec().sample_rate(),
                sender_config.input_sample_spec.sample_rate());
}

TEST(sender_encoder, read) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    SenderEncoder sender_encoder(context, sender_config);
    CHECK(sender_encoder.is_valid());

    packet::PacketPtr pp;

    // TODO(gh-183): compare with StatusNotFound
    CHECK_EQUAL(status::StatusNoData,
                sender_encoder.read(address::Iface_AudioSource, pp));
    CHECK_EQUAL(status::StatusNoData,
                sender_encoder.read(address::Iface_AudioRepair, pp));
}

TEST(sender_encoder, activate_no_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    sender_config.fec_encoder.scheme = packet::FEC_None;

    SenderEncoder sender_encoder(context, sender_config);
    CHECK(sender_encoder.is_valid());
    CHECK(!sender_encoder.is_complete());

    CHECK(sender_encoder.activate(address::Iface_AudioSource, address::Proto_RTP));
    CHECK(sender_encoder.is_complete());
}

TEST(sender_encoder, activate_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    SenderEncoder sender_encoder(context, sender_config);
    CHECK(sender_encoder.is_valid());
    CHECK(!sender_encoder.is_complete());

    if (fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
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
    CHECK(context.is_valid());

    SenderEncoder sender_encoder(context, sender_config);
    CHECK(sender_encoder.is_valid());

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderSessionMetrics sess_metrics;

    CHECK(sender_encoder.get_metrics(slot_metrics, sess_metrics));
    CHECK(!slot_metrics.is_complete);

    CHECK(sender_encoder.activate(address::Iface_AudioSource, address::Proto_RTP));

    CHECK(sender_encoder.get_metrics(slot_metrics, sess_metrics));
    CHECK(slot_metrics.is_complete);
}

} // namespace node
} // namespace roc
