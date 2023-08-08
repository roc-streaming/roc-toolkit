/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_fec/codec_map.h"
#include "roc_peer/context.h"
#include "roc_peer/receiver.h"

namespace roc {
namespace peer {

namespace {

enum { DefaultSlot = 0 };

core::HeapArena arena;

void parse_uri(address::EndpointUri& uri, const char* str) {
    CHECK(address::parse_endpoint_uri(str, address::EndpointUri::Subset_Full, uri));
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

    {
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
}

TEST(receiver, bind_slots) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    {
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

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        netio::UdpReceiverConfig config;
        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, config));

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

        address::EndpointUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(source_endp.port() == 0);
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(source_endp.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);
}

TEST(receiver, configure_slots) {
    Context context(context_config, arena);
    CHECK(context.is_valid());

    UNSIGNED_LONGS_EQUAL(context.network_loop().num_ports(), 0);

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.is_valid());

        netio::UdpReceiverConfig config;
        CHECK(receiver.configure(0, address::Iface_AudioSource, config));
        CHECK(receiver.configure(1, address::Iface_AudioSource, config));

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

} // namespace peer
} // namespace roc
