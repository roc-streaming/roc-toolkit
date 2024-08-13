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

TEST_GROUP(block_writer_reader_peek) {
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

// Check how peek works when there are no losses.
TEST(block_writer_reader_peek, no_losses) {
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

    for (size_t i_block = 0; i_block < 10; ++i_block) {
        generate_packet_block(i_block * NumSourcePackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
        }
        dispatcher.push_stocks();

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p;

            LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
            check_packet(p, NumSourcePackets * i_block + i);
            check_restored(p, false);

            LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
            check_packet(p, NumSourcePackets * i_block + i);
            check_restored(p, false);
        }
    }
}

// Check that peek works with repaired packets.
TEST(block_writer_reader_peek, repairs_in_the_middle_of_block) {
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

    for (size_t i_block = 0; i_block < 10; ++i_block) {
        dispatcher.clear_losses();
        dispatcher.lose(10);
        dispatcher.lose(11);

        generate_packet_block(i_block * NumSourcePackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
        }
        dispatcher.push_stocks();

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p;

            LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
            check_packet(p, NumSourcePackets * i_block + i);
            check_restored(p, i == 10 || i == 11);

            LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
            check_packet(p, NumSourcePackets * i_block + i);
            check_restored(p, i == 10 || i == 11);
        }
    }
}

// Check that peek skips lost packets in the middle of the block.
TEST(block_writer_reader_peek, losses_in_the_middle_of_block) {
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

    for (size_t i_block = 0; i_block < 10; ++i_block) {
        dispatcher.clear_losses();
        dispatcher.lose(10);
        dispatcher.lose(11);
        for (size_t i = 0; i < NumRepairPackets; ++i) {
            dispatcher.lose(NumSourcePackets + i);
        }

        generate_packet_block(i_block * NumSourcePackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
        }
        dispatcher.push_stocks();

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            if (i == 10 || i == 11) {
                packet::PacketPtr p;

                LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
                check_packet(p, NumSourcePackets * i_block + 12);
                check_restored(p, false);
            } else {
                packet::PacketPtr p;

                LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
                check_packet(p, NumSourcePackets * i_block + i);
                check_restored(p, false);

                LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
                check_packet(p, NumSourcePackets * i_block + i);
                check_restored(p, false);
            }
        }
    }
}

// Check that peek skips lost packets in the beginning of the block.
TEST(block_writer_reader_peek, losses_in_the_beginning_of_block) {
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

    for (size_t i_block = 0; i_block < 10; ++i_block) {
        dispatcher.clear_losses();
        if (i_block > 0) {
            dispatcher.lose(0);
            dispatcher.lose(1);
            for (size_t i = 0; i < NumRepairPackets; ++i) {
                dispatcher.lose(NumSourcePackets + i);
            }
        }

        generate_packet_block(i_block * NumSourcePackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
        }
        dispatcher.push_stocks();

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            if (i_block > 0 && (i == 0 || i == 1)) {
                packet::PacketPtr p;

                LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
                check_packet(p, NumSourcePackets * i_block + 2);
                check_restored(p, false);
            } else {
                packet::PacketPtr p;

                LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
                check_packet(p, NumSourcePackets * i_block + i);
                check_restored(p, false);

                LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
                check_packet(p, NumSourcePackets * i_block + i);
                check_restored(p, false);
            }
        }
    }
}

// Check that peek does not move to next block when packet losses are in
// the end of the block, but instead returns StatusDrain.
TEST(block_writer_reader_peek, losses_in_the_end_of_block) {
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

    for (size_t i_block = 0; i_block < 10; ++i_block) {
        dispatcher.clear_losses();
        dispatcher.lose(NumSourcePackets - 2);
        dispatcher.lose(NumSourcePackets - 1);
        for (size_t i = 0; i < NumRepairPackets; ++i) {
            dispatcher.lose(NumSourcePackets + i);
        }

        generate_packet_block(i_block * NumSourcePackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
        }
        dispatcher.push_stocks();

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            if (i == NumSourcePackets - 2 || i == NumSourcePackets - 1) {
                packet::PacketPtr p;

                LONGS_EQUAL(status::StatusDrain, reader.read(p, packet::ModePeek));
                CHECK(!p);
            } else {
                packet::PacketPtr p;

                if (i_block > 0 && i == 0) {
                    LONGS_EQUAL(status::StatusDrain, reader.read(p, packet::ModePeek));
                    CHECK(!p);
                } else {
                    LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
                    check_packet(p, NumSourcePackets * i_block + i);
                    check_restored(p, false);
                }

                LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
                check_packet(p, NumSourcePackets * i_block + i);
                check_restored(p, false);
            }
        }
    }
}

// Peek packet when there is loss, then deliver lost packet and peek again.
TEST(block_writer_reader_peek, late_source_packets) {
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

    // Mark packets 10 and 11 as delayed
    dispatcher.delay(10);
    dispatcher.delay(11);

    // Lose all repair packets to prevent repairing
    for (size_t i = 0; i < NumRepairPackets; ++i) {
        dispatcher.lose(NumSourcePackets + i);
    }

    generate_packet_block(0);

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
    }
    dispatcher.push_stocks();

    // Peek and fetch packets 0-9
    for (size_t i = 0; i < 10; ++i) {
        packet::PacketPtr p;

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
        check_packet(p, i);
        check_restored(p, false);

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        check_packet(p, i);
        check_restored(p, false);
    }

    // Now peek returns packet 12
    {
        packet::PacketPtr p;

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
        check_packet(p, 12);
        check_restored(p, false);
    }

    // Deliver packets 10 and 11
    dispatcher.push_delayed(10);
    dispatcher.push_delayed(11);

    // Peek and fetch packets 10-...
    for (size_t i = 10; i < NumSourcePackets; ++i) {
        packet::PacketPtr p;

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
        check_packet(p, i);
        check_restored(p, false);

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        check_packet(p, i);
        check_restored(p, false);
    }
}

// Peek packet when there is loss, then deliver repair packet and restore losses,
// and then peek again.
TEST(block_writer_reader_peek, late_repair_packets) {
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

    // Mark packets 10 and 11 as lost
    dispatcher.lose(10);
    dispatcher.lose(11);

    // Delay all repair packets to prevent repairing
    for (size_t i = 0; i < NumRepairPackets; ++i) {
        dispatcher.delay(NumSourcePackets + i);
    }

    generate_packet_block(0);

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        LONGS_EQUAL(status::StatusOK, writer.write(source_packets[i]));
    }
    dispatcher.push_stocks();

    // Peek and fetch packets 0-9
    for (size_t i = 0; i < 10; ++i) {
        packet::PacketPtr p;

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
        check_packet(p, i);
        check_restored(p, false);

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        check_packet(p, i);
        check_restored(p, false);
    }

    // Now peek returns packet 12
    {
        packet::PacketPtr p;

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
        check_packet(p, 12);
        check_restored(p, false);
    }

    // Deliver all repair packets to allow repairing
    for (size_t i = 0; i < NumRepairPackets; ++i) {
        dispatcher.push_delayed(NumSourcePackets + i);
    }

    // Peek and fetch packets 10-...
    for (size_t i = 10; i < NumSourcePackets; ++i) {
        packet::PacketPtr p;

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModePeek));
        check_packet(p, i);
        check_restored(p, i == 10 || i == 11);

        LONGS_EQUAL(status::StatusOK, reader.read(p, packet::ModeFetch));
        check_packet(p, i);
        check_restored(p, i == 10 || i == 11);
    }
}

} // namespace fec
} // namespace roc
