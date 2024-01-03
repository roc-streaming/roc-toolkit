/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_fec/codec_map.h"
#include "roc_node/context.h"
#include "roc_node/receiver.h"

namespace roc {
namespace node {

namespace {

enum { DefaultSlot = 0 };

core::HeapArena arena;

void parse_uri(address::EndpointUri& uri, const char* str) {
    CHECK(address::parse_endpoint_uri(str, address::EndpointUri::Subset_Full, uri));
}

void handle_sess_metrics(const pipeline::ReceiverSessionMetrics& sess_metrics,
                         size_t sess_index,
                         void* sess_arg) {
    ((pipeline::ReceiverSessionMetrics*)sess_arg)[sess_index] = sess_metrics;
}

} // namespace

TEST_GROUP(receiver) {
    ContextConfig context_config;
    pipeline::ReceiverConfig receiver_config;
};

TEST(receiver, source) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    Receiver receiver(context, receiver_config);
    CHECK(receiver.is_valid());

    UNSIGNED_LONGS_EQUAL(receiver.source().sample_spec().sample_rate(),
                         receiver_config.common.output_sample_spec.sample_rate());
}

TEST(receiver, bind) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // one slot
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(source_endp.port() == 0);
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(source_endp.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // two slots
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(receiver, configure) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // one slot
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        netio::UdpConfig iface_config;
        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(source_endp.port() == 0);
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(source_endp.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // two slots
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        netio::UdpConfig iface_config;
        CHECK(receiver.configure(0, address::Iface_AudioSource, iface_config));
        CHECK(receiver.configure(1, address::Iface_AudioSource, iface_config));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(receiver, unlink) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // bind one slot, unlink one slot
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(source_endp.port() == 0);
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(source_endp.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        CHECK(receiver.unlink(DefaultSlot));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // bind two slots, unlink one slot
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);

        CHECK(receiver.unlink(0));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    { // bind two slots, unlink two slots
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 2);

        CHECK(receiver.unlink(0));
        CHECK(receiver.unlink(1));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(receiver, endpoints_no_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
    }
}

TEST(receiver, endpoints_fec) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        // fec is not supported
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        // fec is not supported
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        return;
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        // everything is ok
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "ldpc://127.0.0.1:0");

        // repair port fec scheme mismatch
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp+ldpc://127.0.0.1:0");

        // source port fec scheme mismatch
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        address::EndpointUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        // repair port provided when fec is disabled
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));
    }
}

TEST(receiver, bind_errors) {
    { // incomplete endpoint
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        CHECK(source_endp.set_proto(address::Proto_RTP));

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // partially invalidated endpoint
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");
        CHECK(source_endp.set_port(-1));

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // resolve error
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://invalid.:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(receiver, configure_errors) {
    { // multicast group: inappropriate address
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        netio::UdpConfig iface_config;
        strcpy(iface_config.multicast_interface, "8.8.8.8");

        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // multicast group: IP familty mismatch
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        netio::UdpConfig iface_config;
        // set IPv6 group
        strcpy(iface_config.multicast_interface, "::");

        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken());

        address::EndpointUri source_endp(arena);
        // bind t IPv4 address
        parse_uri(source_endp, "rtp://224.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // multicast group: multicast flag mismatch
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        netio::UdpConfig iface_config;
        // set multicast group
        strcpy(iface_config.multicast_interface, "0.0.0.0");

        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken());

        address::EndpointUri source_endp(arena);
        // bind to non-multicast address
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(receiver, flow_errors) {
    { // configure after bind
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        netio::UdpConfig iface_config;
        CHECK(!receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // bind twice
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // unlink non-existent
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        CHECK(!receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
    { // unlink twice
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.has_broken());

        CHECK(receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken());

        CHECK(!receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(receiver, recover) {
    { // rebind after error
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:0");

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // can't bind, slot is broken
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // unlink slot
        CHECK(receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken());

        // can bind
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(!receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }
    { // configure after error
        Context context(context_config, arena);
        CHECK(context.is_valid());

        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        address::EndpointUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:0");

        address::EndpointUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // can't configure, slot is broken
        netio::UdpConfig iface_config;
        CHECK(!receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        // unlink slot
        CHECK(receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken());

        // can configure
        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken());

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
    }
}

TEST(receiver, metrics) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    Receiver receiver(context, receiver_config);
    CHECK(receiver.is_valid());

    pipeline::ReceiverSlotMetrics slot_metrics;
    pipeline::ReceiverSessionMetrics sess_metrics[10];
    size_t sess_metrics_size = 0;

    sess_metrics_size = ROC_ARRAY_SIZE(sess_metrics);
    CHECK(!receiver.get_metrics(DefaultSlot, slot_metrics, handle_sess_metrics,
                                &sess_metrics_size, sess_metrics));

    address::EndpointUri source_endp(arena);
    parse_uri(source_endp, "rtp://127.0.0.1:0");
    CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

    sess_metrics_size = ROC_ARRAY_SIZE(sess_metrics);
    CHECK(receiver.get_metrics(DefaultSlot, slot_metrics, handle_sess_metrics,
                               &sess_metrics_size, sess_metrics));

    UNSIGNED_LONGS_EQUAL(0, slot_metrics.num_sessions);
    UNSIGNED_LONGS_EQUAL(0, sess_metrics_size);
}

} // namespace node
} // namespace roc
