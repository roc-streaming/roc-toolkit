/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_fec/composer.h"
#include "roc_fec/parser.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace fec {

namespace {

const size_t Test_payload_size = 10;

const size_t Test_rtp_source = 0x11223344;
const size_t Test_rtp_seqnum = 0x5566;
const size_t Test_rtp_timestamp = 0x778899aa;
const size_t Test_rtp_pt = 0xb;

const size_t Test_fec_esi = 0x11;
const size_t Test_fec_sbn = 0x2233;
const size_t Test_fec_sbl = 0x4455;
const size_t Test_fec_nes = 0x6677;

const uint8_t Ref_rtp_ldpc_source[] = {
    /* RTP header */
    0x80, 0x0B, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa,
    0x11, 0x22, 0x33, 0x44,
    /* Payload */
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a,
    /* LDPC source footer */
    0x22, 0x33, 0x00, 0x11,
    0x44, 0x55
};

const uint8_t Ref_ldpc_repair[] = {
    /* LDPC repair header */
    0x22, 0x33, 0x00, 0x11,
    0x44, 0x55, 0x66, 0x77,
    /* Payload */
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a
};

const uint8_t Ref_rtp_rsm8_source[] = {
    /* RTP header */
    0x80, 0x0B, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa,
    0x11, 0x22, 0x33, 0x44,
    /* Payload */
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a,
    /* RSm8 footer */
    0x00, 0x22, 0x33, 0x11,
    0x44, 0x55
};

const uint8_t Ref_rsm8_repair[] = {
    /* RSm8 header */
    0x00, 0x22, 0x33, 0x11,
    0x44, 0x55,
    /* Payload */
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a
};

struct PacketTest {
    packet::IComposer* composer;
    packet::IParser* parser;

    packet::FecScheme scheme;
    size_t block_length;

    bool is_rtp;

    const uint8_t* reference;
    size_t reference_size;
};

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, 1000, true);
packet::PacketPool packet_pool(allocator, true);

void fill_packet(packet::Packet& packet, bool is_rtp) {
    if (is_rtp) {
        CHECK(packet.rtp());

        packet.rtp()->source = Test_rtp_source;
        packet.rtp()->seqnum = Test_rtp_seqnum;
        packet.rtp()->timestamp = Test_rtp_timestamp;
        packet.rtp()->payload_type = Test_rtp_pt;
    }

    CHECK(packet.fec());

    packet.fec()->encoding_symbol_id = Test_fec_esi;
    packet.fec()->source_block_number = Test_fec_sbn;
    packet.fec()->source_block_length = Test_fec_sbl;
    packet.fec()->block_length = Test_fec_nes;

    core::Slice<uint8_t> packet_payload;
    if (is_rtp) {
        packet_payload = packet.rtp()->payload;
    } else {
        packet_payload = packet.fec()->payload;
    }

    UNSIGNED_LONGS_EQUAL(Test_payload_size, packet_payload.size());
    for (size_t i = 1; i <= Test_payload_size; i++) {
        packet_payload.data()[i - 1] = uint8_t(i % 255);
    }
}

void check_packet(packet::Packet& packet,
                  packet::FecScheme scheme,
                  size_t block_length,
                  bool is_rtp) {
    if (is_rtp) {
        CHECK(packet.rtp());

        UNSIGNED_LONGS_EQUAL(Test_rtp_source, packet.rtp()->source);
        UNSIGNED_LONGS_EQUAL(Test_rtp_seqnum, packet.rtp()->seqnum);
        UNSIGNED_LONGS_EQUAL(Test_rtp_timestamp, packet.rtp()->timestamp);
        UNSIGNED_LONGS_EQUAL(Test_rtp_pt, packet.rtp()->payload_type);
    }

    CHECK(packet.fec());

    UNSIGNED_LONGS_EQUAL(scheme, packet.fec()->fec_scheme);
    UNSIGNED_LONGS_EQUAL(Test_fec_esi, packet.fec()->encoding_symbol_id);
    UNSIGNED_LONGS_EQUAL(Test_fec_sbn, packet.fec()->source_block_number);
    UNSIGNED_LONGS_EQUAL(Test_fec_sbl, packet.fec()->source_block_length);
    UNSIGNED_LONGS_EQUAL(block_length, packet.fec()->block_length);

    core::Slice<uint8_t> packet_payload;
    if (is_rtp) {
        packet_payload = packet.rtp()->payload;
    } else {
        packet_payload = packet.fec()->payload;
    }

    UNSIGNED_LONGS_EQUAL(Test_payload_size, packet_payload.size());
    for (size_t i = 1; i <= Test_payload_size; i++) {
        UNSIGNED_LONGS_EQUAL(i % 255, packet_payload.data()[i - 1]);
    }
}

void test_compose(const PacketTest& test) {
    core::Slice<uint8_t> buffer = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
    CHECK(buffer);

    packet::PacketPtr packet = new (packet_pool) packet::Packet(packet_pool);
    CHECK(packet);

    CHECK(test.composer->prepare(*packet, buffer, Test_payload_size));

    packet->set_data(buffer);

    fill_packet(*packet, test.is_rtp);

    CHECK(test.composer->compose(*packet));

    UNSIGNED_LONGS_EQUAL(test.reference_size, packet->data().size());
    for (size_t i = 0; i < test.reference_size; i++) {
        UNSIGNED_LONGS_EQUAL(test.reference[i], packet->data().data()[i]);
    }
}

void test_parse(const PacketTest& test) {
    core::Slice<uint8_t> buffer = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
    CHECK(buffer);

    buffer.resize(test.reference_size);
    for (size_t i = 0; i < test.reference_size; i++) {
        buffer.data()[i] = test.reference[i];
    }

    packet::PacketPtr packet = new (packet_pool) packet::Packet(packet_pool);
    CHECK(packet);

    packet->set_data(buffer);

    CHECK(test.parser->parse(*packet, packet->data()));

    check_packet(*packet, test.scheme, test.block_length, test.is_rtp);
}

void test_compose_parse(const PacketTest& test) {
    core::Slice<uint8_t> buffer = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
    CHECK(buffer);

    packet::PacketPtr packet1 = new (packet_pool) packet::Packet(packet_pool);
    CHECK(packet1);

    CHECK(test.composer->prepare(*packet1, buffer, Test_payload_size));

    packet1->set_data(buffer);

    fill_packet(*packet1, test.is_rtp);

    CHECK(test.composer->compose(*packet1));

    packet::PacketPtr packet2 = new (packet_pool) packet::Packet(packet_pool);
    CHECK(packet2);

    CHECK(test.parser->parse(*packet2, packet1->data()));

    check_packet(*packet2, test.scheme, test.block_length, test.is_rtp);
}

void test_all(const PacketTest& test) {
    test_compose(test);
    test_parse(test);
    test_compose_parse(test);
}

} // namespace

TEST_GROUP(composer_parser){};

TEST(composer_parser, rtp_ldpc_source) {
    rtp::Composer rtp_composer(NULL);
    Composer<LDPC_Source_PayloadID, Source, Footer> ldpc_composer(&rtp_composer);

    rtp::FormatMap rtp_format_map;
    rtp::Parser rtp_parser(rtp_format_map, NULL);
    Parser<LDPC_Source_PayloadID, Source, Footer> ldpc_parser(&rtp_parser);

    PacketTest test;
    test.composer = &ldpc_composer;
    test.parser = &ldpc_parser;
    test.scheme = packet::FEC_LDPC_Staircase;
    test.is_rtp = true;
    test.block_length = 0;
    test.reference = Ref_rtp_ldpc_source;
    test.reference_size = sizeof(Ref_rtp_ldpc_source);

    test_all(test);
}

TEST(composer_parser, ldpc_repair) {
    Composer<LDPC_Repair_PayloadID, Repair, Header> ldpc_composer(NULL);
    Parser<LDPC_Repair_PayloadID, Repair, Header> ldpc_parser(NULL);

    PacketTest test;
    test.composer = &ldpc_composer;
    test.parser = &ldpc_parser;
    test.scheme = packet::FEC_LDPC_Staircase;
    test.is_rtp = false;
    test.block_length = Test_fec_nes;
    test.reference = Ref_ldpc_repair;
    test.reference_size = sizeof(Ref_ldpc_repair);

    test_all(test);
}

TEST(composer_parser, rtp_rsm8_source) {
    rtp::Composer rtp_composer(NULL);
    Composer<RSm8_PayloadID, Source, Footer> rsm8_composer(&rtp_composer);

    rtp::FormatMap rtp_format_map;
    rtp::Parser rtp_parser(rtp_format_map, NULL);
    Parser<RSm8_PayloadID, Source, Footer> rsm8_parser(&rtp_parser);

    PacketTest test;
    test.composer = &rsm8_composer;
    test.parser = &rsm8_parser;
    test.scheme = packet::FEC_ReedSolomon_M8;
    test.is_rtp = true;
    test.block_length = 255;
    test.reference = Ref_rtp_rsm8_source;
    test.reference_size = sizeof(Ref_rtp_rsm8_source);

    test_all(test);
}

TEST(composer_parser, rsm8_repair) {
    Composer<RSm8_PayloadID, Repair, Header> rsm8_composer(NULL);
    Parser<RSm8_PayloadID, Repair, Header> rsm8_parser(NULL);

    PacketTest test;
    test.composer = &rsm8_composer;
    test.parser = &rsm8_parser;
    test.scheme = packet::FEC_ReedSolomon_M8;
    test.is_rtp = false;
    test.block_length = 255;
    test.reference = Ref_rsm8_repair;
    test.reference_size = sizeof(Ref_rsm8_repair);

    test_all(test);
}

} // namespace fec
} // namespace roc
