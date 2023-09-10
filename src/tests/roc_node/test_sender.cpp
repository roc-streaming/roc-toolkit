/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/protocol.h"
#include "roc_core/heap_arena.h"
#include "roc_fec/codec_map.h"
#include "roc_node/context.h"
#include "roc_node/sender.h"

namespace roc {
namespace node {

namespace {

enum { DefaultSlot = 0 };

core::HeapArena arena;

void parse_uri(address::EndpointUri& uri, const char* str) {
    CHECK(address::parse_endpoint_uri(str, address::EndpointUri::Subset_Full, uri));
}

} // namespace

TEST_GROUP(sender) {
    ContextConfig context_config;
    pipeline::SenderConfig sender_config;
};

TEST(sender, sink) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    Sender sender(context, sender_config);
    CHECK(sender.is_valid());

    UNSIGNED_LONGS_EQUAL(sender.sink().sample_spec().sample_rate(),
                         sender_config.input_sample_spec.sample_rate());
}

TEST(sender, connect) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // one slot
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // two slots
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:111");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:222");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(sender, configure) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // one slot
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        netio::UdpSenderConfig iface_config;
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // two slots
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        netio::UdpSenderConfig iface_config;
        CHECK(sender.configure(0, address::Iface_AudioSource, iface_config));
        CHECK(sender.configure(1, address::Iface_AudioSource, iface_config));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:111");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:222");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(sender, unlink) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // connect one slot, unlink one slot
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        CHECK(sender.unlink(DefaultSlot));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // connect two slots, unlink one slot
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:111");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:222");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);

        CHECK(sender.unlink(0));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // connect two slots, unlink two slots
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:111");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:222");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);

        CHECK(sender.unlink(0));
        CHECK(sender.unlink(1));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(sender, endpoints_no_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        // everything is ok
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_incomplete());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        // source port not provided
        CHECK(!sender.has_incomplete());
    }
}

TEST(sender, endpoints_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        // fec is not supported
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_incomplete());

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // fec is not supported
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete());

        return;
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // everything is ok
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.has_incomplete());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+ldpc://127.0.0.1:123");

        // source port fec scheme mismatch
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_incomplete());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "ldpc://127.0.0.1:123");

        // repair port fec scheme mismatch
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // repair port provided when fec is disabled
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        // repair port not provided when fec is enabled
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_incomplete());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        // source port not provided when fec is enabled
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete());
    }
}

TEST(sender, endpoints_fec_multiple_slots) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        return;
    }

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    Sender sender(context, sender_config);
    CHECK(sender.is_valid());

    address::EndpointUri source_endp1(arena);
    parse_uri(source_endp1, "rtp+rs8m://127.0.0.1:1001");

    address::EndpointUri repair_endp1(arena);
    parse_uri(repair_endp1, "rs8m://127.0.0.1:1002");

    address::EndpointUri source_endp2(arena);
    parse_uri(source_endp2, "rtp+rs8m://127.0.0.1:2001");

    address::EndpointUri repair_endp2(arena);
    parse_uri(repair_endp2, "rs8m://127.0.0.1:2002");

    CHECK(!sender.has_incomplete());

    CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));
    CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

    CHECK(sender.has_incomplete());

    CHECK(sender.connect(0, address::Iface_AudioRepair, repair_endp1));
    CHECK(sender.connect(1, address::Iface_AudioRepair, repair_endp2));

    CHECK(!sender.has_incomplete());
}

TEST(sender, connect_errors) {
    { // incomplete endpoint
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        CHECK(source_endp.set_proto(address::Proto_RTP));

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // partially invalidated endpoint
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");
        CHECK(source_endp.set_port(-1));

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // incompatible endpoints
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "ldpc://127.0.0.1:123");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // resolve error
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://invalid.:123");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(sender, configure_errors) {
    { // outgoing address: inappropriate address
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        netio::UdpSenderConfig iface_config;
        CHECK(iface_config.bind_address.set_host_port_auto("8.8.8.8", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!sender.has_broken());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // outgoing address: IP family mismatch
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        netio::UdpSenderConfig iface_config;
        CHECK(iface_config.bind_address.set_host_port_auto("::", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!sender.has_broken());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(sender, flow_errors) {
    { // configure after connect
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        netio::UdpSenderConfig iface_config;
        CHECK(!sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // connect twice
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // unlink non-existent
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        CHECK(!sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // unlink twice
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken());

        CHECK(sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken());

        CHECK(!sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(sender, recover) {
    { // reconnect after error
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:123");

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:123");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // can't connect, slot is broken
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // unlink slot
        CHECK(sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken());

        // can connect
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(!sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }
    { // configure after error
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:123");

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:123");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // can't configure, slot is broken
        netio::UdpSenderConfig iface_config;
        CHECK(!sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // unlink slot
        CHECK(sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken());

        // can configure
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!sender.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(sender, port_sharing) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        return;
    }

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    { // source and repair shared: same empty config
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }
    { // source and repair shared: same non-empty config
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        netio::UdpSenderConfig iface_config;
        CHECK(iface_config.bind_address.set_host_port_auto("127.0.0.1", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioRepair, iface_config));

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }
    { // source and repair not shared: different families
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://[::1]:123");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        if (sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp)) {
            // IPv6 may be unsupported
            UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
        }
    }
    { // source and repair not shared: different addresses
        Sender sender(context, sender_config);
        CHECK(sender.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:123");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:123");

        netio::UdpSenderConfig iface_config1;
        CHECK(iface_config1.bind_address.set_host_port_auto("127.0.0.1", 0));

        netio::UdpSenderConfig iface_config2;
        CHECK(iface_config2.bind_address.set_host_port_auto("127.0.0.2", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config1));
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioRepair, iface_config2));

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        if (sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp)) {
            // 127.0.0.2 may be unsupported
            UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
        }
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(sender, metrics) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    Sender sender(context, sender_config);
    CHECK(sender.is_valid());

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderSessionMetrics sess_metrics;

    CHECK(!sender.get_metrics(DefaultSlot, slot_metrics, sess_metrics));

    address::EndpointUri source_endp(arena);
    parse_uri(source_endp, "rtp://127.0.0.1:123");
    CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

    CHECK(sender.get_metrics(DefaultSlot, slot_metrics, sess_metrics));
    CHECK(slot_metrics.is_complete);
}

} // namespace node
} // namespace roc
