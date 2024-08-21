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
#include "roc_node/context.h"
#include "roc_node/sender.h"
#include "roc_packet/fec.h"

namespace roc {
namespace node {

namespace {

enum { DefaultSlot = 0 };

core::HeapArena arena;

void parse_uri(address::NetworkUri& uri, const char* str) {
    CHECK(address::parse_network_uri(str, address::NetworkUri::Subset_Full, uri));
    CHECK(uri.verify(address::NetworkUri::Subset_Full));
}

void write_slot_metrics(const pipeline::SenderSlotMetrics& slot_metrics, void* slot_arg) {
    *(pipeline::SenderSlotMetrics*)slot_arg = slot_metrics;
}

void write_party_metrics(const pipeline::SenderParticipantMetrics& party_metrics,
                         size_t party_index,
                         void* party_arg) {
    ((pipeline::SenderParticipantMetrics*)party_arg)[party_index] = party_metrics;
}

} // namespace

TEST_GROUP(sender) {
    ContextConfig context_config;
    pipeline::SenderSinkConfig sender_config;
};

TEST(sender, sink) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    Sender sender(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    LONGS_EQUAL(sender_config.input_sample_spec.sample_rate(),
                sender.sink().sample_spec().sample_rate());
}

TEST(sender, connect) {
    { // one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // two slots
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:1000");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:2000");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        LONGS_EQUAL(2, context.network_loop().num_ports());
    }
}

TEST(sender, configure) {
    { // one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));

        LONGS_EQUAL(0, context.network_loop().num_ports());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // two slots
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        CHECK(sender.configure(0, address::Iface_AudioSource, iface_config));
        CHECK(sender.configure(1, address::Iface_AudioSource, iface_config));

        LONGS_EQUAL(0, context.network_loop().num_ports());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:1000");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:2000");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        LONGS_EQUAL(2, context.network_loop().num_ports());
    }
}

TEST(sender, unlink) {
    { // connect one slot, unlink one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        CHECK(sender.unlink(DefaultSlot));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // connect two slots, unlink one slot
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:1000");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:2000");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        LONGS_EQUAL(2, context.network_loop().num_ports());

        CHECK(sender.unlink(0));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // connect two slots, unlink two slots
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://127.0.0.1:1000");
        CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:2000");
        CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

        LONGS_EQUAL(2, context.network_loop().num_ports());

        CHECK(sender.unlink(0));
        CHECK(sender.unlink(1));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // slot with 2 endpoints
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1001");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:1002");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        CHECK(sender.unlink(DefaultSlot));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    // slot with 3 endpoints
    if (fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:1003");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        CHECK(sender.unlink(DefaultSlot));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(sender, endpoints_no_fec) {
    { // all good
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_incomplete_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // source endpoint not provided
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        CHECK(!sender.has_incomplete_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(sender, endpoints_fec) {
    // fec not supported
    if (!fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        // fec is not supported
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_incomplete_slots());

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        // fec is not supported
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        return;
    }
    { // all good
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(!sender.has_incomplete_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // fec scheme mismatch (source endpoint)
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+ldpc://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // fec scheme mismatch (repair endpoint)
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "ldpc://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // repair port provided when fec is disabled
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // repair port not provided when fec is enabled
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // source port not provided when fec is enabled
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
}

TEST(sender, endpoints_fec_multiple_slots) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    if (!fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        return;
    }

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    Sender sender(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    address::NetworkUri source_endp1(arena);
    parse_uri(source_endp1, "rtp+rs8m://127.0.0.1:1001");

    address::NetworkUri repair_endp1(arena);
    parse_uri(repair_endp1, "rs8m://127.0.0.1:1002");

    address::NetworkUri source_endp2(arena);
    parse_uri(source_endp2, "rtp+rs8m://127.0.0.1:2001");

    address::NetworkUri repair_endp2(arena);
    parse_uri(repair_endp2, "rs8m://127.0.0.1:2002");

    CHECK(!sender.has_incomplete_slots());

    CHECK(sender.connect(0, address::Iface_AudioSource, source_endp1));
    CHECK(sender.connect(1, address::Iface_AudioSource, source_endp2));

    CHECK(sender.has_incomplete_slots());
    LONGS_EQUAL(2, context.network_loop().num_ports());

    CHECK(sender.connect(0, address::Iface_AudioRepair, repair_endp1));
    CHECK(sender.connect(1, address::Iface_AudioRepair, repair_endp2));

    CHECK(!sender.has_incomplete_slots());
    LONGS_EQUAL(2, context.network_loop().num_ports());
}

TEST(sender, endpoints_control) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    { // control
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // source + control
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1001");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:1002");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));
        CHECK(!sender.has_incomplete_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    // source + repair + control
    if (fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:1003");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));
        CHECK(!sender.has_incomplete_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // protocol mismatch
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtp://127.0.0.1:1001");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));
        CHECK(sender.has_incomplete_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(sender, connect_errors) {
    { // incomplete endpoint
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        CHECK(source_endp.set_proto(address::Proto_RTP));

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // partially invalidated endpoint
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");
        CHECK(source_endp.set_port(-1));

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // incompatible endpoints
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "ldpc://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // resolve error
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://invalid.:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // address already in use
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        address::NetworkUri source_endp(arena);
        address::NetworkUri repair_endp(arena);

        CHECK(iface_config.bind_address.set_host_port_auto("127.0.0.1", 0));
        parse_uri(source_endp, "rtp://127.0.0.1:1001");
        parse_uri(repair_endp, "rtp://127.0.0.1:1002");

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());

        CHECK(iface_config.bind_address.set_host_port_auto("127.0.0.1",
                                                           source_endp.port()));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioRepair, iface_config));
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(sender, configure_errors) {
    { // bind address: inappropriate address
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        CHECK(iface_config.bind_address.set_host_port_auto("8.8.8.8", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!sender.has_broken_slots());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // bind address: IP family mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        CHECK(iface_config.bind_address.set_host_port_auto("::", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!sender.has_broken_slots());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // multicast group: inappropriate address
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        strcpy(iface_config.multicast_interface, "8.8.8.8");

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioControl, iface_config));
        CHECK(!sender.has_broken_slots());

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // multicast group: IP familty mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        // set IPv6 group
        strcpy(iface_config.multicast_interface, "::");

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioControl, iface_config));
        CHECK(!sender.has_broken_slots());

        address::NetworkUri control_endp(arena);
        // connect to IPv4 address
        parse_uri(control_endp, "rtcp://224.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // multicast group: multicast flag mismatch
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        netio::UdpConfig iface_config;
        // set multicast group
        strcpy(iface_config.multicast_interface, "0.0.0.0");

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioControl, iface_config));
        CHECK(!sender.has_broken_slots());

        address::NetworkUri control_endp(arena);
        // connect to non-multicast address
        parse_uri(control_endp, "rtcp://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(sender, flow_errors) {
    { // configure after connect
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());

        netio::UdpConfig iface_config;
        CHECK(!sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // connect twice
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // unlink non-existent
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        CHECK(!sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
    { // unlink twice
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp://127.0.0.1:1000");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(!sender.has_broken_slots());

        CHECK(sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken_slots());

        CHECK(!sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(sender, recover) {
    { // reconnect after error
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:1000");

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // can't connect, slot is broken
        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // unlink slot
        CHECK(sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken_slots());

        // can connect
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp2));
        CHECK(!sender.has_broken_slots());

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // configure after error
        Context context(context_config, arena);
        LONGS_EQUAL(status::StatusOK, context.init_status());

        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp1(arena);
        parse_uri(source_endp1, "rtp://invalid.:1000");

        address::NetworkUri source_endp2(arena);
        parse_uri(source_endp2, "rtp://127.0.0.1:1000");

        CHECK(!sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp1));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // can't configure, slot is broken
        netio::UdpConfig iface_config;
        CHECK(!sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());

        // unlink slot
        CHECK(sender.unlink(DefaultSlot));
        CHECK(!sender.has_broken_slots());

        // can configure
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(!sender.has_broken_slots());

        LONGS_EQUAL(0, context.network_loop().num_ports());
    }
}

TEST(sender, port_sharing) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    LONGS_EQUAL(0, context.network_loop().num_ports());

    if (!fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8)) {
        return;
    }

    sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

    { // source and repair shared: same empty config
        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // source, repair, and control shared: same empty config
        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        address::NetworkUri control_endp(arena);
        parse_uri(control_endp, "rtcp://127.0.0.1:1003");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioControl, control_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // source and repair shared: same non-empty config
        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        netio::UdpConfig iface_config;
        CHECK(iface_config.bind_address.set_host_port_auto("127.0.0.1", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config));
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioRepair, iface_config));

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));
        CHECK(sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp));

        LONGS_EQUAL(1, context.network_loop().num_ports());
    }
    { // source and repair not shared: different families
        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://[::1]:1002");

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        if (sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp)) {
            // IPv6 may be unsupported
            LONGS_EQUAL(2, context.network_loop().num_ports());
        }
    }
    { // source and repair not shared: different addresses
        Sender sender(context, sender_config);
        LONGS_EQUAL(status::StatusOK, sender.init_status());

        address::NetworkUri source_endp(arena);
        parse_uri(source_endp, "rtp+rs8m://127.0.0.1:1001");

        address::NetworkUri repair_endp(arena);
        parse_uri(repair_endp, "rs8m://127.0.0.1:1002");

        netio::UdpConfig iface_config1;
        CHECK(iface_config1.bind_address.set_host_port_auto("127.0.0.1", 0));

        netio::UdpConfig iface_config2;
        CHECK(iface_config2.bind_address.set_host_port_auto("127.0.0.2", 0));

        CHECK(sender.configure(DefaultSlot, address::Iface_AudioSource, iface_config1));
        CHECK(sender.configure(DefaultSlot, address::Iface_AudioRepair, iface_config2));

        CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

        if (sender.connect(DefaultSlot, address::Iface_AudioRepair, repair_endp)) {
            // 127.0.0.2 may be unsupported
            LONGS_EQUAL(2, context.network_loop().num_ports());
        }
    }

    LONGS_EQUAL(0, context.network_loop().num_ports());
}

TEST(sender, metrics) {
    Context context(context_config, arena);
    LONGS_EQUAL(status::StatusOK, context.init_status());

    Sender sender(context, sender_config);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderParticipantMetrics party_metrics[10];
    size_t party_count = 0;

    party_count = ROC_ARRAY_SIZE(party_metrics);
    CHECK(!sender.get_metrics(DefaultSlot, write_slot_metrics, &slot_metrics,
                              write_party_metrics, &party_count, &party_metrics));

    address::NetworkUri source_endp(arena);
    parse_uri(source_endp, "rtp://127.0.0.1:1000");
    CHECK(sender.connect(DefaultSlot, address::Iface_AudioSource, source_endp));

    party_count = ROC_ARRAY_SIZE(party_metrics);
    CHECK(sender.get_metrics(DefaultSlot, write_slot_metrics, &slot_metrics,
                             write_party_metrics, &party_count, &party_metrics));

    LONGS_EQUAL(0, slot_metrics.num_participants);
    LONGS_EQUAL(0, party_count);
}

} // namespace node
} // namespace roc
