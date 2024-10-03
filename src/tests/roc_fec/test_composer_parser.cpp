/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_harness.h"

#include "roc_core/heap_arena.h"
#include "roc_fec/composer.h"
#include "roc_fec/parser.h"
#include "roc_packet/packet_factory.h"
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
    0x80, 0x0B, 0x55, 0x66, //
    0x77, 0x88, 0x99, 0xaa, //
    0x11, 0x22, 0x33, 0x44, //
    /* Payload */
    0x01, 0x02, 0x03, 0x04, //
    0x05, 0x06, 0x07, 0x08, //
    0x09, 0x0a,
    /* LDPC source footer */
    0x22, 0x33, 0x00, 0x11, //
    0x44, 0x55
};

const uint8_t Ref_ldpc_repair[] = {
    /* LDPC repair header */
    0x22, 0x33, 0x00, 0x11, //
    0x44, 0x55, 0x66, 0x77, //
    /* Payload */
    0x01, 0x02, 0x03, 0x04, //
    0x05, 0x06, 0x07, 0x08, //
    0x09, 0x0a
};

const uint8_t Ref_rtp_rs8m_source[] = {
    /* RTP header */
    0x80, 0x0B, 0x55, 0x66, //
    0x77, 0x88, 0x99, 0xaa, //
    0x11, 0x22, 0x33, 0x44, //
    /* Payload */
    0x01, 0x02, 0x03, 0x04, //
    0x05, 0x06, 0x07, 0x08, //
    0x09, 0x0a,
    /* RS8M footer */
    0x00, 0x22, 0x33, 0x11, //
    0x44, 0x55
};

const uint8_t Ref_rs8m_repair[] = {
    /* RS8M header */
    0x00, 0x22, 0x33, 0x11, //
    0x44, 0x55,
    /* Payload */
    0x01, 0x02, 0x03, 0x04, //
    0x05, 0x06, 0x07, 0x08, //
    0x09, 0x0a
};

enum { BufferSize = 1000 };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, BufferSize);

struct PacketTest {
    packet::IComposer* composer;
    packet::IParser* parser;

    packet::FecScheme scheme;
    size_t block_length;

    bool is_rtp;

    const uint8_t* reference;
    size_t reference_size;
};

void fill_packet(packet::Packet& packet, bool is_rtp) {
    if (is_rtp) {
        CHECK(packet.rtp());

        packet.rtp()->source_id = Test_rtp_source;
        packet.rtp()->seqnum = Test_rtp_seqnum;
        packet.rtp()->stream_timestamp = Test_rtp_timestamp;
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

        UNSIGNED_LONGS_EQUAL(Test_rtp_source, packet.rtp()->source_id);
        UNSIGNED_LONGS_EQUAL(Test_rtp_seqnum, packet.rtp()->seqnum);
        UNSIGNED_LONGS_EQUAL(Test_rtp_timestamp, packet.rtp()->stream_timestamp);
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
    core::Slice<uint8_t> buffer = packet_factory.new_packet_buffer();
    CHECK(buffer);

    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    CHECK(test.composer->prepare(*packet, buffer, Test_payload_size));

    packet->set_buffer(buffer);

    fill_packet(*packet, test.is_rtp);

    CHECK(test.composer->compose(*packet));

    UNSIGNED_LONGS_EQUAL(test.reference_size, packet->buffer().size());
    for (size_t i = 0; i < test.reference_size; i++) {
        UNSIGNED_LONGS_EQUAL(test.reference[i], packet->buffer().data()[i]);
    }
}

void test_parse(const PacketTest& test) {
    core::Slice<uint8_t> buffer = packet_factory.new_packet_buffer();
    CHECK(buffer);

    buffer.reslice(0, test.reference_size);
    for (size_t i = 0; i < test.reference_size; i++) {
        buffer.data()[i] = test.reference[i];
    }

    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->set_buffer(buffer);

    CHECK(test.parser->parse(*packet, packet->buffer()) == status::StatusOK);

    check_packet(*packet, test.scheme, test.block_length, test.is_rtp);
}

void test_compose_parse(const PacketTest& test) {
    core::Slice<uint8_t> buffer = packet_factory.new_packet_buffer();
    CHECK(buffer);

    packet::PacketPtr packet1 = packet_factory.new_packet();
    CHECK(packet1);

    CHECK(test.composer->prepare(*packet1, buffer, Test_payload_size));

    packet1->set_buffer(buffer);

    fill_packet(*packet1, test.is_rtp);

    CHECK(test.composer->compose(*packet1));

    packet::PacketPtr packet2 = packet_factory.new_packet();
    CHECK(packet2);

    CHECK(test.parser->parse(*packet2, packet1->buffer()) == status::StatusOK);

    check_packet(*packet2, test.scheme, test.block_length, test.is_rtp);
}

void test_all(const PacketTest& test) {
    test_compose(test);
    test_parse(test);
    test_compose_parse(test);
}

} // namespace

TEST_GROUP(composer_parser) {};

TEST(composer_parser, rtp_ldpc_source) {
    rtp::Composer rtp_composer(NULL, arena);
    Composer<LDPC_Source_PayloadID, Source, Footer> ldpc_composer(&rtp_composer, arena);

    rtp::EncodingMap rtp_encoding_map(arena);
    rtp::Parser rtp_parser(NULL, rtp_encoding_map, arena);
    Parser<LDPC_Source_PayloadID, Source, Footer> ldpc_parser(&rtp_parser, arena);

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
    Composer<LDPC_Repair_PayloadID, Repair, Header> ldpc_composer(NULL, arena);
    Parser<LDPC_Repair_PayloadID, Repair, Header> ldpc_parser(NULL, arena);

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

TEST(composer_parser, rtp_rs8m_source) {
    rtp::Composer rtp_composer(NULL, arena);
    Composer<RS8M_PayloadID, Source, Footer> rs8m_composer(&rtp_composer, arena);

    rtp::EncodingMap rtp_encoding_map(arena);
    rtp::Parser rtp_parser(NULL, rtp_encoding_map, arena);
    Parser<RS8M_PayloadID, Source, Footer> rs8m_parser(&rtp_parser, arena);

    PacketTest test;
    test.composer = &rs8m_composer;
    test.parser = &rs8m_parser;
    test.scheme = packet::FEC_ReedSolomon_M8;
    test.is_rtp = true;
    test.block_length = 255;
    test.reference = Ref_rtp_rs8m_source;
    test.reference_size = sizeof(Ref_rtp_rs8m_source);

    test_all(test);
}

TEST(composer_parser, rs8m_repair) {
    Composer<RS8M_PayloadID, Repair, Header> rs8m_composer(NULL, arena);
    Parser<RS8M_PayloadID, Repair, Header> rs8m_parser(NULL, arena);

    PacketTest test;
    test.composer = &rs8m_composer;
    test.parser = &rs8m_parser;
    test.scheme = packet::FEC_ReedSolomon_M8;
    test.is_rtp = false;
    test.block_length = 255;
    test.reference = Ref_rs8m_repair;
    test.reference_size = sizeof(Ref_rs8m_repair);

    test_all(test);
}

} // namespace fec
} // namespace roc
