/*
 * Copyright (c) 2020 Roc Streaming authors
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

enum { DefaultSlot = 0 };

core::HeapAllocator allocator;

void parse_uri(address::EndpointUri& uri, const char* str) {
    CHECK(address::parse_endpoint_uri(str, address::EndpointUri::Subset_Full, uri));
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

    UNSIGNED_LONGS_EQUAL(sender.sink().sample_spec().sample_rate(),
                         sender_config.input_sample_spec.sample_rate());
}

TEST(sender, connect) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    {
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(sender, connect_slots) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    {
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp1(allocator);
        parse_uri(source_endp1, "rtp://127.0.0.1:111");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        address::EndpointUri source_endp2(allocator);
        parse_uri(source_endp2, "rtp://127.0.0.1:222");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(sender, endpoints_no_fec) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        // everything is ok
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
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

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        // fec is not supported
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.is_ready());

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // fec is not supported
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());

        return;
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // everything is ok
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+ldpc://127.0.0.1:123");

        // source port fec scheme mismatch
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "ldpc://127.0.0.1:123");

        // repair port fec scheme mismatch
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // repair port provided when fec is disabled
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        // repair port not provided when fec is enabled
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.is_ready());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // source port not provided when fec is enabled
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.is_ready());
    }
}

TEST(sender, endpoints_fec_slots) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        return;
    }

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    Sender sender(context, sender_config);
    CHECK(sender.valid());

    address::EndpointUri source_endp1(allocator);
    parse_uri(source_endp1, "rtp+rs8m://127.0.0.1:1001");

    address::EndpointUri repair_endp1(allocator);
    parse_uri(repair_endp1, "rs8m://127.0.0.1:1002");

    address::EndpointUri source_endp2(allocator);
    parse_uri(source_endp2, "rtp+rs8m://127.0.0.1:2001");

    address::EndpointUri repair_endp2(allocator);
    parse_uri(repair_endp2, "rs8m://127.0.0.1:2002");

    CHECK(!sender.is_ready());

    CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));
    CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

    CHECK(!sender.is_ready());

    CHECK(sender.connect(0, address::Iface_AudioRepair, repair_endp1));
    CHECK(sender.connect(1, address::Iface_AudioRepair, repair_endp2));

    CHECK(sender.is_ready());
}

TEST(sender, port_sharing) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        return;
    }

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    { // source and repair shared: same empty config
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }
    { // source and repair shared: same non-empty config
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        CHECK(sender.set_outgoing_address(DefaultSlot, address::Iface_AudioSource,
                                          "127.0.0.1"));
        CHECK(sender.set_outgoing_address(DefaultSlot, address::Iface_AudioRepair,
                                          "127.0.0.1"));

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }
    { // source and repair not shared: different families
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://[::1]:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        if (sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp)) {
            // IPv6 may be unsupported
            UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
        }
    }
    { // source and repair not shared: different addresses
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::EndpointUri source_endp(allocator);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(allocator);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        CHECK(sender.set_outgoing_address(DefaultSlot, address::Iface_AudioSource,
                                          "127.0.0.1"));
        CHECK(sender.set_outgoing_address(DefaultSlot, address::Iface_AudioRepair,
                                          "127.0.0.2"));

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        if (sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp)) {
            // 127.0.0.2 may be unsupported
            UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
        }
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

} // namespace peer
} // namespace roc
