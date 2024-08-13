/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/packet_dispatcher.h"

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/block_reader.h"
#include "roc_fec/block_writer.h"
#include "roc_fec/codec_map.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_fec/parser.h"
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

TEST_GROUP(block_writer_reader_duration) {
    packet::PacketPtr source_packets[NumSourcePackets];

    CodecConfig codec_config;
    BlockWriterConfig writer_config;
    BlockReaderConfig reader_config;

    core::ScopedPtr<IBlockEncoder> encoder;
    core::ScopedPtr<IBlockDecoder> decoder;

    void setup() {
        codec_config.scheme = packet::FEC_ReedSolomon_M8;

        writer_config.n_source_packets = NumSourcePackets;
        writer_config.n_repair_packets = NumRepairPackets;

        if (fec_supported()) {
            encoder.reset(CodecMap::instance().new_block_encoder(codec_config,
                                                                 packet_factory, arena));
            decoder.reset(CodecMap::instance().new_block_decoder(codec_config,
                                                                 packet_factory, arena));
            CHECK(encoder);
            CHECK(decoder);
        }
    }

    bool fec_supported() {
        return CodecMap::instance().has_scheme(codec_config.scheme);
    }

    void generate_packet_block(size_t start_sn) {
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            source_packets[i] = generate_packet(start_sn + i);
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
};

TEST(block_writer_reader_duration, no_losses) {
    if (!fec_supported()) {
        return;
    }

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    for (size_t i_block = 0; i_block < 5; ++i_block) {
        generate_packet_block(i_block * NumSourcePackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
        }
        if (i_block > 0) {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, writer.max_block_duration());
        }
        dispatcher.push_stocks();

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p;
            LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
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

TEST(block_writer_reader_duration, lost_first_packet_in_first_block) {
    if (!fec_supported()) {
        return;
    }

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    // Sending first block except first packet.
    generate_packet_block(0);
    dispatcher.lose(0);
    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
    }

    // Sending 2nd, 3rd and 4th blocks lossless.
    for (size_t i_block = 1; i_block < 4; i_block++) {
        dispatcher.clear_losses();
        generate_packet_block(i_block * NumSourcePackets);
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
        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        if (i < NumSourcePackets * 3 - 1) {
            UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
        } else {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, reader.max_block_duration());
        }
    }
}

TEST(block_writer_reader_duration, lost_first_packet_in_third_block) {
    if (!fec_supported()) {
        return;
    }

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

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
        generate_packet_block(i_block * NumSourcePackets);
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
        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        if (i <= NumSourcePackets * 2 - 1) {
            UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
        } else {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, reader.max_block_duration());
        }
    }
}

TEST(block_writer_reader_duration, lost_almost_every_packet) {
    if (!fec_supported()) {
        return;
    }

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    // Sending first block except first packet.
    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    // Sending 1-4 blocks.
    for (size_t i_block = 0; i_block < 4; i_block++) {
        dispatcher.clear_losses();

        generate_packet_block(i_block * NumSourcePackets);
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
        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10 * i, p->stream_timestamp());
        if (i < 2) {
            UNSIGNED_LONGS_EQUAL(0, reader.max_block_duration());
        } else {
            UNSIGNED_LONGS_EQUAL(NumSourcePackets * 10, reader.max_block_duration());
        }
    }
}

TEST(block_writer_reader_duration, lost_single_block) {
    if (!fec_supported()) {
        return;
    }

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    // Sending first block except first packet.
    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    // Sending 1-5 blocks.
    for (size_t i_block = 0; i_block < 5; i_block++) {
        dispatcher.clear_losses();

        generate_packet_block(i_block * NumSourcePackets);
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
        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
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

TEST(block_writer_reader_duration, resize_block_middle) {
    if (!fec_supported()) {
        return;
    }

    test::PacketDispatcher dispatcher(source_parser, repair_parser, packet_factory,
                                      NumSourcePackets, NumRepairPackets);

    BlockWriter writer(writer_config, codec_config.scheme, *encoder, dispatcher,
                       source_composer, repair_composer, packet_factory, arena);

    BlockReader reader(reader_config, codec_config.scheme, *decoder,
                       dispatcher.source_reader(), dispatcher.repair_reader(), rtp_parser,
                       packet_factory, arena);

    packet::seqnum_t wr_sn = 0;
    size_t sb_len[10] = {
        // 0-2
        NumSourcePackets,
        NumSourcePackets,
        NumSourcePackets,
        // 3-5
        2 * NumSourcePackets,
        2 * NumSourcePackets,
        2 * NumSourcePackets,
        // 6-7
        NumSourcePackets,
        NumSourcePackets,
        // 8-9
        NumSourcePackets,
        NumSourcePackets,
    };

    UNSIGNED_LONGS_EQUAL(0, writer.max_block_duration());
    for (size_t i_block = 0; i_block < 10; i_block++) {
        core::Array<packet::PacketPtr> packets(arena);

        dispatcher.clear_losses();

        if (i_block == 3) {
            LONGS_EQUAL(status::StatusOK,
                        writer.resize(sb_len[i_block], dispatcher.repair_size()));
        } else if (i_block == 6) {
            LONGS_EQUAL(status::StatusOK,
                        writer.resize(sb_len[i_block], dispatcher.repair_size()));
        }
        if (!packets.resize(sb_len[i_block])) {
            FAIL("resize failed");
        }
        for (size_t i = 0; i < sb_len[i_block]; ++i) {
            packets[i] = generate_packet(wr_sn);
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
            LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
            if ((i_block == 2 || i_block == 5 || i_block > 7)
                && i_packet < sb_len[i_block] - 1) {
                UNSIGNED_LONGS_EQUAL(sb_len[i_block] * 10, reader.max_block_duration());
            }
        }
    }
}

} // namespace fec
} // namespace roc
