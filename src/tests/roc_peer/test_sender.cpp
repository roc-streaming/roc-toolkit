/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_fec/codec_map.h"
#include "roc_peer/context.h"
#include "roc_peer/sender.h"

namespace roc {
namespace peer {

namespace {

core::HeapAllocator allocator;

void parse_uri(address::EndpointURI& uri, const char* str) {
    CHECK(address::parse_endpoint_uri(str, address::EndpointURI::Subset_Full, uri));
}

} // namespace

TEST_GROUP(sender) {
    ContextConfig context_config;
    pipeline::SenderConfig sender_config;
};

TEST(sender, sink) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    Sender sender(context, sender_config);
    CHECK(sender.valid());

    UNSIGNED_LONGS_EQUAL(sender.sink().sample_rate(), sender_config.input_sample_rate);
}

TEST(sender, connect) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);

    {
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI source_endp(allocator);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(address::Iface_AudioSource, source_endp));

        UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);
}

TEST(sender, endpoints_no_fec) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI source_endp(allocator);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        // everything is ok
        CHECK(sender.connect(address::Iface_AudioSource, source_endp));
        CHECK(sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        // source port not provided
        CHECK(!sender.is_ready());
    }
}

TEST(sender, endpoints_fec) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        // fec is not supported
        CHECK(!sender.connect(address::Iface_AudioSource, source_endp));
        CHECK(!sender.is_ready());

        address::EndpointURI repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // fec is not supported
        CHECK(!sender.connect(address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());

        return;
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointURI repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // everything is ok
        CHECK(sender.connect(address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(address::Iface_AudioRepair, repair_endp));
        CHECK(sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI source_endp(allocator);
        parse_uri(source_endp, "rtp+ldpc://127.0.0.1:123");

        // source port fec scheme mismatch
        CHECK(!sender.connect(address::Iface_AudioSource, source_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI repair_endp(allocator);
        parse_uri(repair_endp, "ldpc://127.0.0.1:123");

        // repair port fec scheme mismatch
        CHECK(!sender.connect(address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // repair port provided when fec is disabled
        CHECK(!sender.connect(address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        // repair port not provided when fec is enabled
        CHECK(sender.connect(address::Iface_AudioSource, source_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointURI repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // source port not provided when fec is enabled
        CHECK(sender.connect(address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());
    }
}

} // namespace peer
} // namespace roc
