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

#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/block_reader.h"
#include "roc_fec/block_writer.h"
#include "roc_fec/codec_map.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_fec/parser.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
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
rtp::Parser rtp_parser(encoding_map, NULL);

Parser<RS8M_PayloadID, Source, Footer> rs8m_source_parser(&rtp_parser);
Parser<RS8M_PayloadID, Repair, Header> rs8m_repair_parser(NULL);
Parser<LDPC_Source_PayloadID, Source, Footer> ldpc_source_parser(&rtp_parser);
Parser<LDPC_Repair_PayloadID, Repair, Header> ldpc_repair_parser(NULL);

rtp::Composer rtp_composer(NULL);
Composer<RS8M_PayloadID, Source, Footer> rs8m_source_composer(&rtp_composer);
Composer<RS8M_PayloadID, Repair, Header> rs8m_repair_composer(NULL);
Composer<LDPC_Source_PayloadID, Source, Footer> ldpc_source_composer(&rtp_composer);
Composer<LDPC_Repair_PayloadID, Repair, Header> ldpc_repair_composer(NULL);

} // namespace

TEST_GROUP(block_duration) {
    packet::PacketPtr source_packets[NumSourcePackets];

    CodecConfig codec_config;
    BlockWriterConfig writer_config;
    BlockReaderConfig reader_config;

    void setup() {
        writer_config.n_source_packets = NumSourcePackets;
        writer_config.n_repair_packets = NumRepairPackets;
    }

    packet::IParser& source_parser() {
        switch (codec_config.scheme) {
        case packet::FEC_ReedSolomon_M8:
            return rs8m_source_parser;
        case packet::FEC_LDPC_Staircase:
            return ldpc_source_parser;
        default:
            roc_panic("bad scheme");
        }
    }

    packet::IParser& repair_parser() {
        switch (codec_config.scheme) {
        case packet::FEC_ReedSolomon_M8:
            return rs8m_repair_parser;
        case packet::FEC_LDPC_Staircase:
            return ldpc_repair_parser;
        default:
            roc_panic("bad scheme");
        }
    }

    packet::IComposer& source_composer() {
        switch (codec_config.scheme) {
        case packet::FEC_ReedSolomon_M8:
            return rs8m_source_composer;
        case packet::FEC_LDPC_Staircase:
            return ldpc_source_composer;
        default:
            roc_panic("bad scheme");
        }
    }

    packet::IComposer& repair_composer() {
        switch (codec_config.scheme) {
        case packet::FEC_ReedSolomon_M8:
            return rs8m_repair_composer;
        case packet::FEC_LDPC_Staircase:
            return ldpc_repair_composer;
        default:
            roc_panic("bad scheme");
        }
    }

    void recompose_packet(const packet::PacketPtr& p) {
        if (p->flags() & packet::Packet::FlagRepair) {
            CHECK(repair_composer().compose(*p));
        } else {
            CHECK(source_composer().compose(*p));
        }
    }

    void fill_all_packets(size_t sn) {
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            source_packets[i] = fill_one_packet(sn + i);
        }
    }

    packet::PacketPtr fill_one_packet(size_t sn, size_t fec_payload_size = FECPayloadSize,
                                      packet::IComposer* composer = NULL) {
        CHECK(fec_payload_size > sizeof(rtp::Header));
        const size_t rtp_payload_size = fec_payload_size - sizeof(rtp::Header);

        packet::PacketPtr pp = packet_factory.new_packet();
        CHECK(pp);

        core::Slice<uint8_t> bp = packet_factory.new_packet_buffer();
        CHECK(bp);

        if (!composer) {
            composer = &source_composer();
        }
        CHECK(composer->prepare(*pp, bp, rtp_payload_size));

        pp->set_buffer(bp);

        UNSIGNED_LONGS_EQUAL(rtp_payload_size, pp->rtp()->payload.size());
        UNSIGNED_LONGS_EQUAL(fec_payload_size, pp->fec()->payload.size());

        pp->add_flags(packet::Packet::FlagAudio | packet::Packet::FlagPrepared);

        pp->rtp()->source_id = SourceID;
        pp->rtp()->payload_type = PayloadType;
        pp->rtp()->seqnum = packet::seqnum_t(sn);
        pp->rtp()->stream_timestamp = packet::stream_timestamp_t(sn * 10);

        for (size_t i = 0; i < rtp_payload_size; i++) {
            pp->rtp()->payload.data()[i] = uint8_t(sn + i);
        }

        return pp;
    }

    void check_audio_packet(packet::PacketPtr pp, size_t sn,
                            size_t fec_payload_size = FECPayloadSize) {
        CHECK(fec_payload_size > sizeof(rtp::Header));
        const size_t rtp_payload_size = fec_payload_size - sizeof(rtp::Header);

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
        UNSIGNED_LONGS_EQUAL(rtp_payload_size, pp->rtp()->payload.size());

        for (size_t i = 0; i < rtp_payload_size; i++) {
            UNSIGNED_LONGS_EQUAL(uint8_t(sn + i), pp->rtp()->payload.data()[i]);
        }
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

TEST(block_duration, no_losses) {
    if (CodecMap::instance().num_schemes() == 0) {
        return;
    }

    const size_t n_blocks = 5;
    codec_config.scheme = CodecMap::instance().nth_scheme(0);

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_encoder(codec_config, packet_factory, arena), arena);

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_decoder(codec_config, packet_factory, arena), arena);

    test::PacketDispatcher dispatcher(source_parser(), repair_parser(), packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer(), repair_composer(), packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    for (size_t i_block = 0; i_block < n_blocks; ++i_block) {
        fill_all_packets(i_block * NumSourcePackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
        }
        if (i_block > 0) {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, writer.max_block_duration());
        }
        dispatcher.push_stocks();

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p;
            LONGS_EQUAL(status::StatusOK, reader.read(p));
            if (i_block == 0) {
                UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
            } else {
                CHECK(reader.is_started());
                if (i_block > 1) {
                    UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10,
                                         reader.max_block_duration());
                }
            }
        }
    }
}

TEST(block_duration, lost_first_packet_in_first_block) {
    if (CodecMap::instance().num_schemes() == 0) {
        return;
    }

    codec_config.scheme = CodecMap::instance().nth_scheme(0);

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_encoder(codec_config, packet_factory, arena), arena);

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_decoder(codec_config, packet_factory, arena), arena);

    test::PacketDispatcher dispatcher(source_parser(), repair_parser(), packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer(), repair_composer(), packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    // Sending first block except first packet.
    fill_all_packets(0);
    dispatcher.lose(0);
    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
    }

    // Sending 2nd, 3rd and 4th blocks lossless.
    for (size_t i_block = 1; i_block < 4; i_block++) {
        dispatcher.clear_losses();
        fill_all_packets(i_block * NumSourcePackets);
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK,
                        writer.write(source_packets[i % NumSourcePackets]));
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, writer.max_block_duration());
        }
        dispatcher.push_stocks();
    }

    // Receive every sent packet except the first one.
    for (size_t i = 1; i < NumSourcePackets * 4; ++i) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, reader.read(p));
        if (i < NumSourcePackets * 3 - 1) {
            UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
        } else {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, reader.max_block_duration());
        }
    }
}

TEST(block_duration, lost_first_packet_in_third_block) {
    if (CodecMap::instance().num_schemes() == 0) {
        return;
    }

    codec_config.scheme = CodecMap::instance().nth_scheme(0);

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_encoder(codec_config, packet_factory, arena), arena);

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_decoder(codec_config, packet_factory, arena), arena);

    test::PacketDispatcher dispatcher(source_parser(), repair_parser(), packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer(), repair_composer(), packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    // Sending first block except first packet.
    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    // Sending 1-4 blocks.
    for (size_t i_block = 0; i_block < 4; i_block++) {
        if (i_block == 2) {
            dispatcher.lose(0);
        } else {
            dispatcher.clear_losses();
        }
        fill_all_packets(i_block * NumSourcePackets);
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK,
                        writer.write(source_packets[i % NumSourcePackets]));
            if (i_block > 0) {
                UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, writer.max_block_duration());
            }
        }
        dispatcher.push_stocks();
    }

    // Receive every sent packet except the first one.
    for (size_t i = 1; i < NumSourcePackets * 4; ++i) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, reader.read(p));
        if (i <= NumSourcePackets * 2 - 1) {
            UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
        } else {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, reader.max_block_duration());
        }
    }
}

TEST(block_duration, lost_almost_every_packet) {
    if (CodecMap::instance().num_schemes() == 0) {
        return;
    }

    codec_config.scheme = CodecMap::instance().nth_scheme(0);

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_encoder(codec_config, packet_factory, arena), arena);

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_decoder(codec_config, packet_factory, arena), arena);

    test::PacketDispatcher dispatcher(source_parser(), repair_parser(), packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer(), repair_composer(), packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    // Sending first block except first packet.
    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    // Sending 1-4 blocks.
    for (size_t i_block = 0; i_block < 4; i_block++) {
        dispatcher.clear_losses();

        fill_all_packets(i_block * NumSourcePackets);
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            if (i > 0) {
                dispatcher.lose(i);
            }
            LONGS_EQUAL(status::StatusOK,
                        writer.write(source_packets[i % NumSourcePackets]));
            if (i_block > 0) {
                UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, writer.max_block_duration());
            }
        }
        dispatcher.push_stocks();
    }

    // Receive every sent packet except the first one.
    for (size_t i = 0; i < 4; ++i) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, reader.read(p));
        UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10 * i, p->stream_timestamp());
        if (i < 2) {
            UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
        } else {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, reader.max_block_duration());
        }
    }
}

TEST(block_duration, lost_single_block) {
    if (CodecMap::instance().num_schemes() == 0) {
        return;
    }

    codec_config.scheme = CodecMap::instance().nth_scheme(0);

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_encoder(codec_config, packet_factory, arena), arena);

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_decoder(codec_config, packet_factory, arena), arena);

    test::PacketDispatcher dispatcher(source_parser(), repair_parser(), packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer(), repair_composer(), packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    // Sending first block except first packet.
    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    // Sending 1-5 blocks.
    for (size_t i_block = 0; i_block < 5; i_block++) {
        dispatcher.clear_losses();

        fill_all_packets(i_block * NumSourcePackets);
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            if (i_block == 3) {
                dispatcher.lose(i);
            }
            LONGS_EQUAL(status::StatusOK,
                        writer.write(source_packets[i % NumSourcePackets]));
            if (i_block > 0) {
                UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, writer.max_block_duration());
            }
        }
        dispatcher.push_stocks();
    }

    // Receive every sent packet except the first one.
    for (size_t i = 0; i < 4 * NumSourcePackets; ++i) {
        packet::PacketPtr p;
        LONGS_EQUAL(status::StatusOK, reader.read(p));
        if (i >= 3 * NumSourcePackets) {
            UNSIGNED_LONGS_EQUAL(10 * (i + NumSourcePackets), p->stream_timestamp());
        } else {
            UNSIGNED_LONGS_EQUAL(10 * i, p->stream_timestamp());
        }
        if (i < 2 * NumSourcePackets - 1) {
            UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
        } else {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, reader.max_block_duration());
        }
    }
}

TEST(block_duration, resize_block_middle) {
    if (CodecMap::instance().num_schemes() == 0) {
        return;
    }

    codec_config.scheme = CodecMap::instance().nth_scheme(0);

    core::ScopedPtr<IBlockEncoder> encoder(
        CodecMap::instance().new_encoder(codec_config, packet_factory, arena), arena);

    core::ScopedPtr<IBlockDecoder> decoder(
        CodecMap::instance().new_decoder(codec_config, packet_factory, arena), arena);

    test::PacketDispatcher dispatcher(source_parser(), repair_parser(), packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer(), repair_composer(), packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    packet::seqnum_t wr_sn = 0;
    size_t sb_len[10] = { NumSourcePackets,     NumSourcePackets,
                          NumSourcePackets, // 0-2
                          2 * NumSourcePackets, 2 * NumSourcePackets,
                          2 * NumSourcePackets,                     // 3-5
                          NumSourcePackets,     NumSourcePackets,   // 6-7
                          NumSourcePackets,     NumSourcePackets }; // 8-9

    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    for (size_t i_block = 0; i_block < 10; i_block++) {
        core::Array<packet::PacketPtr> packets(arena);

        dispatcher.clear_losses();

        if (i_block == 3) {
            writer.resize(sb_len[i_block], dispatcher.repair_size());
        } else if (i_block == 6) {
            writer.resize(sb_len[i_block], dispatcher.repair_size());
        }
        if (!packets.resize(sb_len[i_block])) {
            FAIL("resize failed");
        }
        for (size_t i = 0; i < sb_len[i_block]; ++i) {
            packets[i] = fill_one_packet(wr_sn, FECPayloadSize);
            wr_sn++;

            LONGS_EQUAL(status::StatusOK, writer.write(packets[i]));
        }
        dispatcher.push_stocks();
        if (i_block >= 4) {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 2 * 10, writer.max_block_duration());
        } else if (i_block > 0) {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, writer.max_block_duration());
        }
    }

    // Receive every sent packet except the first one.
    for (size_t i_block = 0; i_block < 10; ++i_block) {
        packet::PacketPtr p;
        for (size_t i_packet = 0; i_packet < sb_len[i_block]; i_packet++) {
            LONGS_EQUAL(status::StatusOK, reader.read(p));
            if ((i_block == 2 || i_block == 5 || i_block > 7)
                && i_packet < sb_len[i_block] - 1) {
                UNSIGNED_LONGS_EQUAL(sb_len[i_block] * 10, reader.max_block_duration());
            }
        }
    }
}

} // namespace fec
} // namespace roc
