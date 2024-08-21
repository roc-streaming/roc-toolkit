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
#include "roc_core/macro_helpers.h"
#include "roc_fec/codec_map.h"
#include "roc_node/context.h"
#include "roc_node/receiver.h"

namespace roc {
namespace node {

namespace {

enum { DefaultSlot = 0 };

core::HeapArena arena;

void parse_uri(address::NetworkUri& uri, const char* str) {
    CHECK(address::parse_network_uri(str, address::NetworkUri::Subset_Full, uri));
    CHECK(uri.verify(address::NetworkUri::Subset_Full));
}

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

TEST_GROUP(receiver) {
    ContextConfig context_config;
    pipeline::ReceiverSourceConfig receiver_config;
};

TEST(receiver, source) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    Receiver receiver(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    LONGS_EQUAL(receiver_config.common.output_sample_spec.sample_rate(),
                receiver.source().sample_spec().sample_rate());
}

TEST(receiver, bind) {
    { // one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(source_endp.port() == 0);
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(source_endp.port() != 0);

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // two slots
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        LONGS_EQUAL(2, context.network_loop().num_ports());
    }
}

TEST(receiver, configure) {
    { // one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        netio::UdpConfig iface_config;
        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));

        LONGS_EQUAL(0, context.network_loop().num_ports());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(source_endp.port() == 0);
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(source_endp.port() != 0);

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // two slots
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        netio::UdpConfig iface_config;
        CHECK(receiver.configure(0, address::Iface_AudioSource, iface_config));
        CHECK(receiver.configure(1, address::Iface_AudioSource, iface_config));

        LONGS_EQUAL(0, context.network_loop().num_ports());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        LONGS_EQUAL(2, context.network_loop().num_ports());
    }
}

TEST(receiver, unlink) {
    { // bind one slot, unlink one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(source_endp.port() == 0);
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(source_endp.port() != 0);

        LONGS_EQUAL(1, context.network_loop().num_ports());

        CHECK(receiver.unlink(DefaultSlot));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // bind two slots, unlink one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        LONGS_EQUAL(2, context.network_loop().num_ports());

        CHECK(receiver.unlink(0));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // bind two slots, unlink two slots
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:0");

        CHECK(source_endp1.port() == 0);
        CHECK(receiver.bind(0, address::Iface_AudioSource, source_endp1));
        CHECK(source_endp1.port() != 0);

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(source_endp2.port() == 0);
        CHECK(receiver.bind(1, address::Iface_AudioSource, source_endp2));
        CHECK(source_endp2.port() != 0);

        LONGS_EQUAL(2, context.network_loop().num_ports());

        CHECK(receiver.unlink(0));
        CHECK(receiver.unlink(1));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // slot with 2 endpoints
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(2, context.network_loop().num_ports());

        CHECK(receiver.unlink(DefaultSlot));
    }
    // slot with 3 endpoints
    if (fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(3, context.network_loop().num_ports());

        CHECK(receiver.unlink(DefaultSlot));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(receiver, endpoints_no_fec) {
    { // all good
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
}

TEST(receiver, endpoints_fec) {
    // fec not supported
    if (!fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        // fec is not supported
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        // fec is not supported
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        LONGS_EQUAL(0, context.network_loop().num_ports());

        return;
    }
    { // all good
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        LONGS_EQUAL(2, context.network_loop().num_ports());
    }
    { // repair port fec scheme mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "ldpc://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // source port fec scheme mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+ldpc://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // repair port provided when fec is disabled
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(receiver, endpoints_control) {
    { // control
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // source + control
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(2, context.network_loop().num_ports());
    }
    // source + repair + control
    if (fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:0");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:0");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(3, context.network_loop().num_ports());
    }
    { // protocol mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(receiver, bind_errors) {
    { // incomplete endpoint
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        CHECK(source_endp.set_proto(address::Proto_RTP));

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // partially invalidated endpoint
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");
        CHECK(source_endp.set_port(-1));

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // resolve error
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://invalid.:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // address already in use
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        address::NetworkUri control_endp(arena);
        CHECK(control_endp.set_proto(address::Proto_RTCP));
        CHECK(control_endp.set_host("127.0.0.1"));
        CHECK(control_endp.set_port(source_endp.port()));

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(receiver, configure_errors) {
    { // multicast group: inappropriate address
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        netio::UdpConfig iface_config;
        strcpy(iface_config.multicast_interface, "8.8.8.8");

        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken_slots());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // multicast group: IP family mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        netio::UdpConfig iface_config;
        // set IPv6 group
        strcpy(iface_config.multicast_interface, "::");

        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken_slots());

        address::NetworkUri source_endp(arena);
        // bind to IPv4 address
        parse_uri(source_endp, "rtp://224.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // multicast group: multicast flag mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        netio::UdpConfig iface_config;
        // set multicast group
        strcpy(iface_config.multicast_interface, "0.0.0.0");

        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken_slots());

        address::NetworkUri source_endp(arena);
        // bind to non-multicast address
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(receiver, flow_errors) {
    { // configure after bind
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.has_broken_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());

        netio::UdpConfig iface_config;
        CHECK(!receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // bind twice
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.has_broken_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // unlink non-existent
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        CHECK(!receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // unlink twice
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:0");

        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!receiver.has_broken_slots());

        CHECK(receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken_slots());

        CHECK(!receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(receiver, recover) {
    { // rebind after error
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:0");

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // can't bind, slot is broken
        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // unlink slot
        CHECK(receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken_slots());

        // can bind
        CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(!receiver.has_broken_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // configure after error
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Receiver receiver(context, receiver_config);
        LONGS_EQUAL(status::StatusOK, receiver.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:0");

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:0");

        CHECK(!receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // can't configure, slot is broken
        netio::UdpConfig iface_config;
        CHECK(!receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // unlink slot
        CHECK(receiver.unlink(DefaultSlot));
        CHECK(!receiver.has_broken_slots());

        // can configure
        CHECK(receiver.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!receiver.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(receiver, metrics) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    Receiver receiver(context, receiver_config);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    pipeline::ReceiverSlotMetrics slot_metrics;
    pipeline::ReceiverParticipantMetrics party_metrics[10];
    size_t party_count = 0;

    party_count = ROC_ARRAY_SIZE(party_metrics);
    CHECK(!receiver.get_metrics(DefaultSlot, write_slot_metrics, &slot_metrics,
                                write_party_metrics, &party_count, &party_metrics));

    address::NetworkUri source_endp(arena);
    parse_uri(source_endp, "rtp://127.0.0.1:0");
    CHECK(receiver.bind(DefaultSlot, address::Iface_AudioSource, source_endp));

    party_count = ROC_ARRAY_SIZE(party_metrics);
    CHECK(receiver.get_metrics(DefaultSlot, write_slot_metrics, &slot_metrics,
                               write_party_metrics, &party_count, &party_metrics));

    LONGS_EQUAL(0, slot_metrics.num_participants);
    LONGS_EQUAL(0, party_count);
}

} // namespace node
} // namespace roc
