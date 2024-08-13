/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_arena.h"
#include "test_helpers/packet_dispatcher.h"
#include "test_helpers/status_reader.h"

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/block_reader.h"
#include "roc_fec/block_writer.h"
#include "roc_fec/codec_map.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_fec/parser.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace fec {

namespace {

const size_t NumSourcePackets = 20;
const size_t NumRepairPackets = 10;

const unsigned SourceID = 555;
const unsigned PayloadType = rtp::PayloadType_L16_Stereo;

const size_t FECPayloadSize = 193;

const size_t MaxBuffSize = 500;

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, MaxBuffSize);

rtp::EncodingMap encoding_map(arena);
rtp::Parser rtp_parser(NULL, encoding_map, arena);

Parser<RS8M_PayloadID, Source, Footer> source_parser(&rtp_parser, arena);
Parser<RS8M_PayloadID, Repair, Header> repair_parser(NULL, arena);

rtp::Composer rtp_composer(NULL, arena);
Composer<RS8M_PayloadID, Source, Footer> source_composer(&rtp_composer, arena);
Composer<RS8M_PayloadID, Repair, Header> repair_composer(NULL, arena);

} // namespace

TEST_GROUP(block_writer_reader_errors) {
    packet::PacketPtr source_packets[NumSourcePackets];

    CodecConfig codec_config;
    BlockWriterConfig writer_config;
    BlockReaderConfig reader_config;

    void setup() {
        codec_config.scheme = packet::FEC_ReedSolomon_M8;

        writer_config.n_source_packets = NumSourcePackets;
        writer_config.n_repair_packets = NumRepairPackets;
    }

    bool fec_supported() {
        return CodecMap::instance().has_scheme(codec_config.scheme);
    }

    void generate_packet_block(size_t sn) {
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            source_packets[i] = generate_packet(sn + i);
        }
    }

    packet::PacketPtr generate_packet(size_t sn) {
        packet::PacketPtr pp = packet_factory.new_packet();
        CHECK(pp);

        core::Slice<uint8_t> bp = packet_factory.new_packet_buffer();
        CHECK(bp);

        CHECK(source_composer.prepare(*pp, bp, FECPayloadSize - sizeof(rtp::Header)));
        pp->set_buffer(bp);

        pp->add_flags(packet::Packet::FlagAudio | packet::Packet::FlagPrepared);

        pp->rtp()->source_id = SourceID;
        pp->rtp()->payload_type = PayloadType;
        pp->rtp()->seqnum = packet::seqnum_t(sn);
        pp->rtp()->stream_timestamp = packet::stream_timestamp_t(sn * 10);

        return pp;
    }

    void check_packet(packet::PacketPtr pp, size_t sn) {
        CHECK(pp);

        CHECK(pp->flags() & packet::Packet::FlagRTP);
        CHECK(pp->flags() & packet::Packet::FlagAudio);

        CHECK(pp->rtp());
        CHECK(pp->rtp()->header);
        CHECK(pp->rtp()->payload);

        UNSIGNED_LONGS_EQUAL(SourceID, pp->rtp()->source_id);

        UNSIGNED_LONGS_EQUAL(sn, pp->rtp()->seqnum);
        UNSIGNED_LONGS_EQUAL(packet::stream_timestamp_t(sn * 10),
                             pp->rtp()->stream_timestamp);

        UNSIGNED_LONGS_EQUAL(PayloadType, pp->rtp()->payload_type);
        UNSIGNED_LONGS_EQUAL(FECPayloadSize - sizeof(rtp::Header),
                             pp->rtp()->payload.size());
    }

    void check_restored(packet::PacketPtr p, bool restored) {
        if (restored) {
            CHECK((p->flags() & packet::Packet::FlagRestored) != 0);
            CHECK(!p->fec());
        } else {
            CHECK((p->flags() & packet::Packet::FlagRestored) == 0);
            CHECK(p->fec());
        }
    }
};

TEST(block_writer_reader_errors, writer_cant_resize_block) {
    enum { BlockSize1 = 50, BlockSize2 = 60 };

    if (!fec_supported()) {
        return;
    }

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_block_encoder(codec_config, packet_factory, arena));
    CHECK(encoder);

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    test::MockArena mock_arena;

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, mock_arena);

    LONGS_EQUAL(status::StatusOK, writer.init_status());

    size_t sn = 0;

    LONGS_EQUAL(status::StatusOK, writer.resize(NumSourcePackets, BlockSize1));

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(generate_packet(sn++)));
    }

    UNSIGNED_LONGS_EQUAL(NumSourcePackets, dispatcher.source_size());
    UNSIGNED_LONGS_EQUAL(BlockSize1, dispatcher.repair_size());

    dispatcher.push_stocks();
    dispatcher.reset();

    mock_arena.set_fail(true);

    LONGS_EQUAL(status::StatusOK, writer.resize(NumSourcePackets, BlockSize2));
    LONGS_EQUAL(status::StatusNoMem, writer.write(generate_packet(sn++)));

    UNSIGNED_LONGS_EQUAL(0, dispatcher.source_size());
    UNSIGNED_LONGS_EQUAL(0, dispatcher.repair_size());
}

TEST(block_writer_reader_errors, writer_cant_encode_packet) {
    enum { BlockSize1 = 50, BlockSize2 = 60 };

    if (!fec_supported()) {
        return;
    }

    test::MockArena mock_arena;

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_block_encoder(codec_config, packet_factory, mock_arena));
    CHECK(encoder);

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    LONGS_EQUAL(status::StatusOK, writer.init_status());

    size_t sn = 0;

    LONGS_EQUAL(status::StatusOK, writer.resize(BlockSize1, NumRepairPackets));

    for (size_t i = 0; i < BlockSize1; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(generate_packet(sn++)));
    }

    UNSIGNED_LONGS_EQUAL(BlockSize1, dispatcher.source_size());
    UNSIGNED_LONGS_EQUAL(NumRepairPackets, dispatcher.repair_size());

    mock_arena.set_fail(true);
    LONGS_EQUAL(status::StatusOK, writer.resize(BlockSize2, NumRepairPackets));
    LONGS_EQUAL(status::StatusNoMem, writer.write(generate_packet(sn++)));

    UNSIGNED_LONGS_EQUAL(BlockSize1, dispatcher.source_size());
    UNSIGNED_LONGS_EQUAL(NumRepairPackets, dispatcher.repair_size());
}

TEST(block_writer_reader_errors, reader_cant_resize_block) {
    enum { BlockSize1 = 50, BlockSize2 = 60 };

    if (!fec_supported()) {
        return;
    }

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_block_encoder(codec_config, packet_factory, arena));

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_block_decoder(codec_config, packet_factory, arena));

    CHECK(encoder);
    CHECK(decoder);

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    test::MockArena mock_arena;

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, mock_arena);

    LONGS_EQUAL(status::StatusOK, writer.init_status());
    LONGS_EQUAL(status::StatusOK, reader.init_status());

    size_t sn = 0;

    // write first block
    LONGS_EQUAL(status::StatusOK, writer.resize(BlockSize1, NumRepairPackets));
    for (size_t i = 0; i < BlockSize1; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(generate_packet(sn++)));
    }

    // deliver first block
    dispatcher.push_stocks();

    // read first block
    for (size_t i = 0; i < BlockSize1; ++i) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        CHECK(p);
        check_packet(p, i);
        check_restored(p, false);
    }

    // write second block
    LONGS_EQUAL(status::StatusOK, writer.resize(BlockSize2, NumRepairPackets));
    for (size_t i = 0; i < BlockSize2; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(generate_packet(sn++)));
    }

    // deliver second block
    dispatcher.push_stocks();

    // configure arena to return errors
    mock_arena.set_fail(true);

    // reader should get an error from arena when trying
    // to resize the block and shut down
    packet::PacketPtr pp;
    LONGS_EQUAL(status::StatusNoMem, reader.read(pp, packet::ModeFetch));
    CHECK(!pp);
}

TEST(block_writer_reader_errors, reader_cant_decode_packet) {
    enum { BlockSize1 = 50, BlockSize2 = 60 };

    if (!fec_supported()) {
        return;
    }

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_block_encoder(codec_config, packet_factory, arena));
    CHECK(encoder);

    test::MockArena mock_arena;

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_block_decoder(codec_config, packet_factory, mock_arena));
    CHECK(decoder);

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    LONGS_EQUAL(status::StatusOK, writer.init_status());
    LONGS_EQUAL(status::StatusOK, reader.init_status());

    size_t sn = 0;

    // write first block
    LONGS_EQUAL(status::StatusOK, writer.resize(BlockSize1, NumRepairPackets));
    for (size_t i = 0; i < BlockSize1; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(generate_packet(sn++)));
    }

    // deliver first block
    dispatcher.push_stocks();

    // read first block
    for (size_t i = 0; i < BlockSize1; ++i) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        CHECK(p);
        check_packet(p, i);
        check_restored(p, false);
    }

    // lose one packet in second block
    dispatcher.reset();
    dispatcher.lose(10);

    // write second block
    LONGS_EQUAL(status::StatusOK, writer.resize(BlockSize2, NumRepairPackets));
    for (size_t i = 0; i < BlockSize2; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(generate_packet(sn++)));
    }

    // deliver second block
    dispatcher.push_stocks();

    // read second block packets before loss
    for (size_t i = 0; i < 10; ++i) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        CHECK(p);
        check_packet(p, BlockSize1 + i);
        check_restored(p, false);
    }

    // configure arena to return errors
    mock_arena.set_fail(true);

    // reader should get an error from arena when trying
    // to repair lost packet and shut down
    packet::PacketPtr pp;
    LONGS_EQUAL(status::StatusNoMem, reader.read(pp, packet::ModeFetch));
    CHECK(!pp);
}

TEST(block_writer_reader_errors, reader_cant_read_source_packet) {
    if (!fec_supported()) {
        return;
    }

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_block_encoder(codec_config, packet_factory, arena));

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_block_decoder(codec_config, packet_factory, arena));

    CHECK(encoder);
    CHECK(decoder);

    packet::FifoQueue writer_queue;
    test::StatusReader source_reader(status::StatusAbort);
    packet::FifoQueue repair_reader;

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, writer_queue,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder, source_reader,
                       repair_reader, rtp_parser, packet_factory, arena);

    LONGS_EQUAL(status::StatusOK, writer.init_status());
    LONGS_EQUAL(status::StatusOK, reader.init_status());

    generate_packet_block(0);
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
    }

    for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
        packet::PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, writer_queue.read(pp, packet::ModeFetch));
        CHECK(pp);

        if (pp->flags() & packet::Packet::FlagRepair) {
            LONGS_EQUAL(status::StatusOK, repair_reader.write(pp));
        }
    }

    packet::PacketPtr pp;
    LONGS_EQUAL(status::StatusAbort, reader.read(pp, packet::ModeFetch));
    CHECK(!pp);
}

TEST(block_writer_reader_errors, reader_cant_read_repair_packet) {
    if (!fec_supported()) {
        return;
    }

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_block_encoder(codec_config, packet_factory, arena));

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_block_decoder(codec_config, packet_factory, arena));

    CHECK(encoder);
    CHECK(decoder);

    packet::FifoQueue writer_queue;
    packet::FifoQueue source_reader;
    test::StatusReader repair_reader(status::StatusAbort);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, writer_queue,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder, source_reader,
                       repair_reader, rtp_parser, packet_factory, arena);

    LONGS_EQUAL(status::StatusOK, writer.init_status());
    LONGS_EQUAL(status::StatusOK, reader.init_status());

    generate_packet_block(0);
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
    }

    for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
        packet::PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, writer_queue.read(pp, packet::ModeFetch));
        CHECK(pp);

        if (pp->flags() & packet::Packet::FlagAudio) {
            LONGS_EQUAL(status::StatusOK, source_reader.write(pp));
        }
    }

    packet::PacketPtr pp;
    LONGS_EQUAL(status::StatusAbort, reader.read(pp, packet::ModeFetch));
    CHECK(!pp);
}

TEST(block_writer_reader_errors, reader_cant_read_source_and_repair_packets) {
    if (!fec_supported()) {
        return;
    }

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_block_decoder(codec_config, packet_factory, arena));

    CHECK(decoder);

    test::StatusReader source_reader(status::StatusAbort);
    test::StatusReader repair_reader(status::StatusAbort);

    BlockReader reader(reader_config, codec_config.scheme, *decoder, source_reader,
                       repair_reader, rtp_parser, packet_factory, arena);

    LONGS_EQUAL(status::StatusOK, reader.init_status());

    packet::PacketPtr pp;
    LONGS_EQUAL(status::StatusAbort, reader.read(pp, packet::ModeFetch));
    CHECK(!pp);
}

} // namespace fec
} // namespace roc
