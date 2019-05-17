/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_fec_schemes.h"
#include "test_mock_allocator.h"
#include "test_packet_dispatcher.h"

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/unique_ptr.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_fec/of_decoder.h"
#include "roc_fec/of_encoder.h"
#include "roc_fec/reader.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/queue.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace fec {

namespace {

const size_t NumSourcePackets = 20;
const size_t NumRepairPackets = 10;

const unsigned SourceID = 555;
const unsigned PayloadType = rtp::PayloadType_L16_Stereo;

const size_t RTPPayloadSize = 177;
const size_t FECPayloadSize = RTPPayloadSize + sizeof(rtp::Header);

const size_t MaxBuffSize = 500;

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBuffSize, true);
packet::PacketPool packet_pool(allocator, true);

rtp::FormatMap format_map;
rtp::Parser rtp_parser(format_map, NULL);
rtp::Composer rtp_composer(NULL);
fec::Composer<RSm8_PayloadID, Source, Footer> rs8m_source_composer(&rtp_composer);
fec::Composer<RSm8_PayloadID, Repair, Header> rs8m_repair_composer(NULL);
fec::Composer<LDPC_Source_PayloadID, Source, Footer> ldpc_source_composer(&rtp_composer);
fec::Composer<LDPC_Repair_PayloadID, Repair, Header> ldpc_repair_composer(NULL);

} // namespace

TEST_GROUP(writer_reader) {
    packet::PacketPtr source_packets[NumSourcePackets];

    Config config;

    void setup() {
        config.n_source_packets = NumSourcePackets;
        config.n_repair_packets = NumRepairPackets;
    }

    packet::IComposer& source_composer() {
        switch ((size_t)config.scheme) {
        case packet::FEC_ReedSolomon_M8:
            return rs8m_source_composer;
        case packet::FEC_LDPC_Staircase:
            return ldpc_source_composer;
        default:
            roc_panic("bad scheme");
        }
    }

    packet::IComposer& repair_composer() {
        switch ((size_t)config.scheme) {
        case packet::FEC_ReedSolomon_M8:
            return rs8m_repair_composer;
        case packet::FEC_LDPC_Staircase:
            return ldpc_repair_composer;
        default:
            roc_panic("bad scheme");
        }
    }

    void fill_all_packets(size_t sn) {
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            source_packets[i] = fill_one_packet(sn + i);
        }
    }

    packet::PacketPtr fill_one_packet(size_t sn) {
        packet::PacketPtr pp = new (packet_pool) packet::Packet(packet_pool);
        CHECK(pp);

        core::Slice<uint8_t> bp = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        CHECK(bp);

        CHECK(source_composer().prepare(*pp, bp, RTPPayloadSize));

        pp->set_data(bp);

        UNSIGNED_LONGS_EQUAL(RTPPayloadSize, pp->rtp()->payload.size());
        UNSIGNED_LONGS_EQUAL(FECPayloadSize, pp->fec()->payload.size());

        pp->add_flags(packet::Packet::FlagAudio);

        pp->rtp()->source = SourceID;
        pp->rtp()->payload_type = PayloadType;
        pp->rtp()->seqnum = packet::seqnum_t(sn);
        pp->rtp()->timestamp = packet::timestamp_t(sn * 10);

        for (size_t i = 0; i < RTPPayloadSize; i++) {
            pp->rtp()->payload.data()[i] = uint8_t(sn + i);
        }

        return pp;
    }

    void check_audio_packet(packet::PacketPtr pp, size_t sn) {
        CHECK(pp);

        CHECK(pp->flags() & packet::Packet::FlagRTP);
        CHECK(pp->flags() & packet::Packet::FlagAudio);

        CHECK(pp->rtp());
        CHECK(pp->rtp()->header);
        CHECK(pp->rtp()->payload);

        UNSIGNED_LONGS_EQUAL(SourceID, pp->rtp()->source);

        UNSIGNED_LONGS_EQUAL(sn, pp->rtp()->seqnum);
        UNSIGNED_LONGS_EQUAL(packet::timestamp_t(sn * 10), pp->rtp()->timestamp);

        UNSIGNED_LONGS_EQUAL(PayloadType, pp->rtp()->payload_type);
        UNSIGNED_LONGS_EQUAL(RTPPayloadSize, pp->rtp()->payload.size());

        for (size_t i = 0; i < RTPPayloadSize; i++) {
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

TEST(writer_reader, no_losses) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }
        dispatcher.push_written();

        CHECK(dispatcher.source_size() == NumSourcePackets);
        CHECK(dispatcher.repair_size() == NumRepairPackets);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }
    }
}

TEST(writer_reader, 1_loss) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        dispatcher.lose(11);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }
        dispatcher.push_written();

        LONGS_EQUAL(NumSourcePackets - 1, dispatcher.source_size());
        LONGS_EQUAL(NumRepairPackets, dispatcher.repair_size());

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, i == 11);
        }
    }
}

TEST(writer_reader, lose_first_packet_in_first_block) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        // Sending first block except first packet.
        fill_all_packets(0);
        dispatcher.lose(0);
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        // Sending second block lossless.
        dispatcher.clear_losses();
        fill_all_packets(NumSourcePackets);
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }
        dispatcher.push_written();

        // Receive every sent packet except the first one.
        for (size_t i = 1; i < NumSourcePackets * 2; ++i) {
            packet::PacketPtr p = reader.read();
            if (i < NumSourcePackets) {
                CHECK(!reader.started());
            } else {
                CHECK(reader.started());
            }
            check_audio_packet(p, i);
            check_restored(p, false);
        }
        CHECK(dispatcher.source_size() == 0);
    }
}

TEST(writer_reader, multiple_blocks_1_loss) {
    enum { NumBlocks = 40 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        for (size_t block_num = 0; block_num < NumBlocks; ++block_num) {
            size_t lost_sq = size_t(-1);
            if (block_num != 5 && block_num != 21 && block_num != 22) {
                lost_sq = (block_num + 1) % (NumSourcePackets + NumRepairPackets);
                dispatcher.lose(lost_sq);
            }

            fill_all_packets(NumSourcePackets * block_num);

            for (size_t i = 0; i < NumSourcePackets; ++i) {
                writer.write(source_packets[i]);
            }
            dispatcher.push_written();

            if (lost_sq == size_t(-1)) {
                CHECK(dispatcher.source_size() == NumSourcePackets);
                CHECK(dispatcher.repair_size() == NumRepairPackets);
            } else if (lost_sq < NumSourcePackets) {
                CHECK(dispatcher.source_size() == NumSourcePackets - 1);
                CHECK(dispatcher.repair_size() == NumRepairPackets);
            } else {
                CHECK(dispatcher.source_size() == NumSourcePackets);
                CHECK(dispatcher.repair_size() == NumRepairPackets - 1);
            }

            for (size_t i = 0; i < NumSourcePackets; ++i) {
                packet::PacketPtr p = reader.read();
                CHECK(p);

                check_audio_packet(p, NumSourcePackets * block_num + i);

                if (lost_sq == size_t(-1)) {
                    check_restored(p, false);
                } else {
                    check_restored(p,
                                   i == lost_sq % (NumSourcePackets + NumRepairPackets));
                }
            }

            dispatcher.reset();
        }
    }
}

TEST(writer_reader, multiple_blocks_in_queue) {
    enum { NumBlocks = 3 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        for (size_t block_num = 0; block_num < NumBlocks; ++block_num) {
            fill_all_packets(NumSourcePackets * block_num);

            for (size_t i = 0; i < NumSourcePackets; ++i) {
                writer.write(source_packets[i]);
            }
        }
        dispatcher.push_written();

        CHECK(dispatcher.source_size() == NumSourcePackets * NumBlocks);
        CHECK(dispatcher.repair_size() == NumRepairPackets * NumBlocks);

        for (size_t block_num = 0; block_num < NumBlocks; ++block_num) {
            for (size_t i = 0; i < NumSourcePackets; ++i) {
                packet::PacketPtr p = reader.read();
                CHECK(p);
                check_audio_packet(p, NumSourcePackets * block_num + i);
                check_restored(p, false);
            }

            dispatcher.reset();
        }
    }
}

TEST(writer_reader, interleaved_packets) {
    enum { NumPackets = NumSourcePackets * 30 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        packet::Interleaver intrlvr(dispatcher, allocator, 10);

        CHECK(intrlvr.valid());

        Writer writer(config, FECPayloadSize, encoder, intrlvr, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        packet::PacketPtr many_packets[NumPackets];

        for (size_t i = 0; i < NumPackets; ++i) {
            many_packets[i] = fill_one_packet(i);
            writer.write(many_packets[i]);
        }
        dispatcher.push_written();

        intrlvr.flush();

        for (size_t i = 0; i < NumPackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }
    }
}

TEST(writer_reader, delayed_packets) {
    // 1. Deliver first half of block.
    // 2. Read first half of block.
    // 3. Try to read more and get NULL.
    // 4. Deliver second half of block.
    // 5. Read second half of block.
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        CHECK(NumSourcePackets > 10);

        // deliver 10 packets to reader
        for (size_t i = 0; i < 10; ++i) {
            dispatcher.push_one_source();
        }

        // read 10 packets
        for (size_t i = 0; i < 10; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        // the rest packets are "delayed" and were not delivered to reader
        // try to read 11th packet and get NULL
        CHECK(!reader.read());

        // deliver "delayed" packets
        dispatcher.push_written();

        // successfully read packets starting from the 11th packet
        for (size_t i = 10; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }
    }
}

TEST(writer_reader, late_out_of_order_packets) {
    // 1. Send a block, but delay some packets in the middle of the block.
    // 2. Read first part of the block before delayed packets.
    // 3. Deliver all delayed packets except one.
    // 4. Read second part of the block.
    // 5. Deliver the last delayed packet.
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        // Mark packets 7-10 as delayed
        dispatcher.clear_delays();
        for (size_t i = 7; i <= 10; ++i) {
            dispatcher.delay(i);
        }

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        // Deliver packets 0-6 and 11-20
        dispatcher.push_written();
        CHECK(dispatcher.source_size() == NumSourcePackets - (10 - 7 + 1));

        // Read packets 0-6
        for (size_t i = 0; i < 7; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        // Deliver packets 7-9
        dispatcher.push_delayed(7);
        dispatcher.push_delayed(8);
        dispatcher.push_delayed(9);

        for (size_t i = 7; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);

            // packets 7-9 were out of order but not late and should be read
            // packet 10 was out of order and late and should be repaired
            check_restored(p, i == 10);

            // Deliver packet 10 (reader should throw it away)
            if (i == 10) {
                dispatcher.push_delayed(10);
            }
        }

        LONGS_EQUAL(0, dispatcher.source_size());
    }
}

TEST(writer_reader, repaired_bad_source_id) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        // change packet source id before passing it to writer
        source_packets[5]->rtp()->source += 1;

        // lose packet with bad source id
        dispatcher.lose(5);

        // encode packets
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        // deliver all packets except the packet with bad source id
        dispatcher.push_written();

        // read packets before the bad packet
        for (size_t i = 0; i < 5; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        // try to read more packets
        // the reader should repair the lost packet, see bad source id, and shutdown
        for (size_t i = 5; i < NumSourcePackets; ++i) {
            CHECK(!reader.read());
            CHECK(!reader.alive());
        }

        CHECK(dispatcher.source_size() == 0);
    }
}

TEST(writer_reader, multiple_repair_attempts) {
    // 1. Lose two distant packets and hold every fec packets in first block,
    //    receive second full block.
    // 2. Detect first loss.
    // 3. Transmit fec packets.
    // 4. Check remaining data packets including lost one.
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        dispatcher.lose(5);
        dispatcher.lose(15);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
            dispatcher.push_one_source();
        }

        dispatcher.clear_losses();

        fill_all_packets(NumSourcePackets);
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
            dispatcher.push_one_source();
        }

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            if (i != 5 && i != 15) {
                packet::PacketPtr p = reader.read();
                CHECK(p);
                check_audio_packet(p, i);
                check_restored(p, false);
            } else if (i == 15) {
                // The moment of truth. Deliver FEC packets accumulated in dispatcher.
                // Reader must try to decode once more.
                dispatcher.push_written();

                packet::PacketPtr p = reader.read();
                CHECK(p);
                check_audio_packet(p, i);
                check_restored(p, true);
            } else if (i == 5) {
                // nop
            }
        }

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i + NumSourcePackets);
            check_restored(p, false);
        }

        LONGS_EQUAL(0, dispatcher.source_size());
    }
}

TEST(writer_reader, drop_outdated_block) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        // Send first block.
        fill_all_packets(NumSourcePackets);
        for (size_t n = 0; n < NumSourcePackets; ++n) {
            writer.write(source_packets[n]);
        }

        // Send outdated block.
        fill_all_packets(0);
        for (size_t n = 0; n < NumSourcePackets; ++n) {
            writer.write(source_packets[n]);
        }

        // Send next block.
        fill_all_packets(NumSourcePackets * 2);
        for (size_t n = 0; n < NumSourcePackets; ++n) {
            writer.write(source_packets[n]);
        }

        dispatcher.push_written();

        // Read first block.
        const packet::PacketPtr first_packet = reader.read();
        CHECK(first_packet);

        const packet::blknum_t sbn = first_packet->fec()->source_block_number;

        for (size_t n = 1; n < NumSourcePackets; ++n) {
            const packet::PacketPtr p = reader.read();
            CHECK(p);

            CHECK(p->fec()->source_block_number == sbn);
        }

        // Read second block.
        for (size_t n = 0; n < NumSourcePackets; ++n) {
            const packet::PacketPtr p = reader.read();
            CHECK(p);

            CHECK(p->fec()->source_block_number == sbn + 1);
        }
    }
}

TEST(writer_reader, repaired_block_numbering) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        const size_t lost_packet_n = 7;

        // Write first block lossy.
        fill_all_packets(0);
        dispatcher.lose(lost_packet_n);

        for (size_t n = 0; n < NumSourcePackets; ++n) {
            writer.write(source_packets[n]);
        }

        dispatcher.clear_losses();

        // Write second block lossless.
        fill_all_packets(NumSourcePackets);

        for (size_t n = 0; n < NumSourcePackets; ++n) {
            writer.write(source_packets[n]);
        }

        dispatcher.push_written();

        // Read first block.
        const packet::PacketPtr first_packet = reader.read();
        CHECK(first_packet);

        const packet::blknum_t sbn = first_packet->fec()->source_block_number;

        for (size_t n = 1; n < NumSourcePackets; ++n) {
            const packet::PacketPtr p = reader.read();
            CHECK(p);

            check_audio_packet(p, n);
            check_restored(p, n == lost_packet_n);

            if (n != lost_packet_n) {
                CHECK(p->fec());
                CHECK(p->fec()->source_block_number == sbn);
            } else {
                CHECK(!p->fec());
            }
        }

        // Read second block.
        for (size_t n = 0; n < NumSourcePackets; ++n) {
            const packet::PacketPtr p = reader.read();
            CHECK(p);

            check_audio_packet(p, NumSourcePackets + n);
            check_restored(p, false);

            CHECK(p->fec());
            CHECK(p->fec()->source_block_number == sbn + 1);
        }
    }
}

TEST(writer_reader, invalid_esi) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        packet::Queue queue;
        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, queue, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        // encode packets and write to queue
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        // write packets from queue to dispatcher
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            packet::PacketPtr p = queue.read();
            CHECK(p);
            if (i == 5) {
                // violates: ESI < SBL (for source packets)
                p->fec()->encoding_symbol_id = NumSourcePackets;
            }
            if (i == NumSourcePackets + 3) {
                // violates: ESI >= SBL (for repair packets)
                p->fec()->encoding_symbol_id = NumSourcePackets - 1;
            }
            if (i == NumSourcePackets + 5) {
                // violates: ESI < NES (for repair packets)
                p->fec()->encoding_symbol_id = NumSourcePackets + NumRepairPackets;
            }
            dispatcher.write(p);
        }

        // deliver packets from dispatcher to reader
        dispatcher.push_written();

        // read packets
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            // packet #5 should be dropped and repaired
            check_restored(p, i == 5);
        }

        CHECK(reader.alive());
        CHECK(dispatcher.source_size() == 0);
        CHECK(dispatcher.repair_size() == 0);
    }
}

TEST(writer_reader, invalid_sbl) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        packet::Queue queue;
        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, queue, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        // encode packets and write to queue
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        // write packets from queue to dispatcher
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            packet::PacketPtr p = queue.read();
            CHECK(p);
            if (i == 5) {
                // violates: SBL can't change in the middle of a block (source packet)
                p->fec()->source_block_length = NumSourcePackets + 1;
            }
            if (i == NumSourcePackets + 3) {
                // violates: SBL can't change in the middle of a block (repair packet)
                p->fec()->source_block_length = NumSourcePackets + 1;
            }
            dispatcher.write(p);
        }

        // deliver packets from dispatcher to reader
        dispatcher.push_written();

        // read packets
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            // packet #5 should be dropped and repaired
            check_restored(p, i == 5);
        }

        CHECK(reader.alive());
        CHECK(dispatcher.source_size() == 0);
        CHECK(dispatcher.repair_size() == 0);
    }
}

TEST(writer_reader, invalid_nes) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        packet::Queue queue;
        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, queue, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        // encode packets and write to queue
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        // write packets from queue to dispatcher
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            packet::PacketPtr p = queue.read();
            CHECK(p);
            if (i == NumSourcePackets) {
                // violates: SBL <= NES
                p->fec()->block_length = NumSourcePackets - 1;
            }
            if (i == NumSourcePackets + 3) {
                // violates: NES can't change in the middle of a block
                p->fec()->block_length = NumSourcePackets + NumRepairPackets + 1;
            }
            dispatcher.write(p);
        }

        // deliver packets from dispatcher to reader
        dispatcher.push_written();

        // read packets
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        CHECK(reader.alive());
        CHECK(dispatcher.source_size() == 0);
        CHECK(dispatcher.repair_size() == 0);
    }
}

TEST(writer_reader, sbn_jump) {
    enum { MaxSbnJump = 30 };

    config.max_sbn_jump = MaxSbnJump;

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        packet::Queue queue;
        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, queue, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        // write three blocks to the queue
        for (size_t n = 0; n < 3; n++) {
            fill_all_packets(NumSourcePackets * n);

            for (size_t i = 0; i < NumSourcePackets; ++i) {
                writer.write(source_packets[i]);
            }
        }

        // write first block to the dispatcher
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            packet::PacketPtr p = queue.read();
            CHECK(p);
            dispatcher.write(p);
        }

        // deliver packets from dispatcher to reader
        dispatcher.push_written();

        // read first block
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        CHECK(reader.alive());

        // write second block to the dispatcher
        // shift it ahead but in the allowed range
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            packet::PacketPtr p = queue.read();
            CHECK(p);
            p->fec()->source_block_number += MaxSbnJump;
            dispatcher.write(p);
        }

        // deliver packets from dispatcher to reader
        dispatcher.push_written();

        // read second block
        for (size_t i = NumSourcePackets; i < NumSourcePackets * 2; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        CHECK(reader.alive());

        // write third block to the dispatcher
        // shift it ahead too far
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            packet::PacketPtr p = queue.read();
            CHECK(p);
            p->fec()->source_block_number += MaxSbnJump * 2 + 1;
            dispatcher.write(p);
        }

        // deliver packets from dispatcher to reader
        dispatcher.push_written();

        // the reader should detect sbn jump and shutdown
        CHECK(!reader.read());
        CHECK(!reader.alive());

        CHECK(dispatcher.source_size() == 0);
        CHECK(dispatcher.repair_size() == 0);
    }
}

TEST(writer_reader, writer_encode_blocks) {
    enum { NumBlocks = 3 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        packet::source_t data_source = 555;

        for (size_t n = 0; n < 5; n++) {
            OFEncoder encoder(config, FECPayloadSize, allocator);

            CHECK(encoder.valid());

            PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

            Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                          repair_composer(), packet_pool, buffer_pool, allocator);

            CHECK(writer.valid());

            packet::blknum_t fec_sbn = 0;

            for (size_t block_num = 0; block_num < NumBlocks; ++block_num) {
                size_t encoding_symbol_id = 0;

                fill_all_packets(NumSourcePackets * block_num);

                for (size_t i = 0; i < NumSourcePackets; ++i) {
                    source_packets[i]->rtp()->source = data_source;
                }

                for (size_t i = 0; i < NumSourcePackets; ++i) {
                    writer.write(source_packets[i]);
                }
                dispatcher.push_written();

                if (block_num == 0) {
                    const packet::FEC* fec = dispatcher.repair_head()->fec();
                    CHECK(fec);

                    fec_sbn = fec->source_block_number;
                }

                for (size_t i = 0; i < NumSourcePackets; ++i) {
                    const packet::PacketPtr p = dispatcher.source_reader().read();
                    CHECK(p);

                    const packet::RTP* rtp = p->rtp();
                    CHECK(rtp);

                    LONGS_EQUAL(data_source, rtp->source);

                    const packet::FEC* fec = p->fec();
                    CHECK(fec);

                    LONGS_EQUAL(fec_sbn, fec->source_block_number);
                    CHECK(fec->source_block_length == NumSourcePackets);
                    UNSIGNED_LONGS_EQUAL(encoding_symbol_id, fec->encoding_symbol_id);

                    encoding_symbol_id++;
                }

                for (size_t i = 0; i < NumRepairPackets; ++i) {
                    const packet::PacketPtr p = dispatcher.repair_reader().read();
                    CHECK(p);

                    const packet::RTP* rtp = p->rtp();
                    CHECK(!rtp);

                    const packet::FEC* fec = p->fec();
                    CHECK(fec);

                    LONGS_EQUAL(fec_sbn, fec->source_block_number);
                    CHECK(fec->source_block_length == NumSourcePackets);
                    UNSIGNED_LONGS_EQUAL(encoding_symbol_id, fec->encoding_symbol_id);

                    encoding_symbol_id++;
                }

                fec_sbn++;
            }

            dispatcher.reset();
        }
    }
}

TEST(writer_reader, writer_resize_blocks) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        CHECK(encoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        CHECK(writer.valid());

        const size_t block_sizes[] = { 15, 25, 35, 43, 33, 23, 13 };

        packet::seqnum_t pack_n = 0;

        for (size_t n = 0; n < ROC_ARRAY_SIZE(block_sizes); ++n) {
            CHECK(writer.resize(block_sizes[n]));

            core::Array<packet::PacketPtr> packets(allocator);
            packets.resize(block_sizes[n]);

            for (size_t i = 0; i < block_sizes[n]; ++i) {
                packets[i] = fill_one_packet(pack_n);
                pack_n++;
            }
            for (size_t i = 0; i < block_sizes[n]; ++i) {
                writer.write(packets[i]);
            }

            CHECK(dispatcher.source_size() == block_sizes[n]);
            CHECK(dispatcher.repair_size() == NumRepairPackets);

            dispatcher.push_written();
            dispatcher.reset();
        }
    }
}

TEST(writer_reader, resize_block_begin) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(decoder.valid());
        CHECK(encoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);
        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(reader.valid());
        CHECK(writer.valid());

        const size_t block_sizes[] = {
            15, 25, 35, 43, 33, 23, 13,
            encoder.max_block_length() - NumRepairPackets
        };

        packet::seqnum_t wr_sn = 0;
        packet::seqnum_t rd_sn = 0;

        for (size_t n = 0; n < ROC_ARRAY_SIZE(block_sizes); ++n) {
            CHECK(writer.resize(block_sizes[n]));

            core::Array<packet::PacketPtr> packets(allocator);
            packets.resize(block_sizes[n]);

            for (size_t i = 0; i < block_sizes[n]; ++i) {
                packets[i] = fill_one_packet(wr_sn);
                wr_sn++;
            }

            for (size_t i = 0; i < block_sizes[n]; ++i) {
                writer.write(packets[i]);
            }

            dispatcher.push_written();

            for (size_t i = 0; i < block_sizes[n]; ++i) {
                const packet::PacketPtr p = reader.read();

                CHECK(p);
                CHECK(p->fec());
                CHECK(p->fec()->source_block_length == block_sizes[n]);

                check_audio_packet(p, rd_sn);
                check_restored(p, false);

                rd_sn++;
            }
        }

        UNSIGNED_LONGS_EQUAL(wr_sn, rd_sn);
    }
}

TEST(writer_reader, resize_block_middle) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(decoder.valid());
        CHECK(encoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);
        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(reader.valid());
        CHECK(writer.valid());

        const size_t block_sizes[] = {
            15, 25, 35, 43, 33, 23, 13,
            encoder.max_block_length() - NumRepairPackets
        };

        size_t prev_sblen = NumSourcePackets;

        packet::seqnum_t wr_sn = 0;
        packet::seqnum_t rd_sn = 0;

        for (size_t n = 0; n < ROC_ARRAY_SIZE(block_sizes); ++n) {
            core::Array<packet::PacketPtr> packets(allocator);
            packets.resize(prev_sblen);

            for (size_t i = 0; i < prev_sblen; ++i) {
                packets[i] = fill_one_packet(wr_sn);
                wr_sn++;
            }

            // Write first half of the packets.
            for (size_t i = 0; i < prev_sblen / 2; ++i) {
                writer.write(packets[i]);
            }

            // Update source block size.
            CHECK(writer.resize(block_sizes[n]));

            // Write the remaining packets.
            for (size_t i = prev_sblen / 2; i < prev_sblen; ++i) {
                writer.write(packets[i]);
            }

            dispatcher.push_written();

            for (size_t i = 0; i < prev_sblen; ++i) {
                const packet::PacketPtr p = reader.read();

                CHECK(p);
                CHECK(p->fec());
                CHECK(p->fec()->source_block_length == prev_sblen);

                check_audio_packet(p, rd_sn);
                check_restored(p, false);

                rd_sn++;
            }

            prev_sblen = block_sizes[n];
        }

        UNSIGNED_LONGS_EQUAL(wr_sn, rd_sn);
    }
}

TEST(writer_reader, resize_block_losses) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(decoder.valid());
        CHECK(encoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);
        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(reader.valid());
        CHECK(writer.valid());

        const size_t block_sizes[] = { 15, 25, 35, 43, 33, 23, 13 };

        packet::seqnum_t wr_sn = 0;
        packet::seqnum_t rd_sn = 0;

        for (size_t n = 0; n < ROC_ARRAY_SIZE(block_sizes); ++n) {
            CHECK(writer.resize(block_sizes[n]));

            core::Array<packet::PacketPtr> packets(allocator);
            packets.resize(block_sizes[n]);

            for (size_t i = 0; i < block_sizes[n]; ++i) {
                packets[i] = fill_one_packet(wr_sn);
                wr_sn++;
            }

            dispatcher.lose(block_sizes[n] / 2);

            for (size_t i = 0; i < block_sizes[n]; ++i) {
                writer.write(packets[i]);
            }

            dispatcher.push_written();

            for (size_t i = 0; i < block_sizes[n]; ++i) {
                const packet::PacketPtr p = reader.read();
                CHECK(p);

                check_audio_packet(p, rd_sn);
                check_restored(p, i == block_sizes[n] / 2);

                rd_sn++;
            }

            dispatcher.reset();
        }

        UNSIGNED_LONGS_EQUAL(wr_sn, rd_sn);
    }
}

TEST(writer_reader, error_writer_encode_packet) {
    enum { BlockSize1 = 50, BlockSize2 = 60 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        MockAllocator mock_allocator;

        OFEncoder encoder(config, FECPayloadSize, mock_allocator);
        CHECK(encoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        CHECK(writer.valid());

        size_t sn = 0;

        CHECK(writer.resize(BlockSize1));

        for (size_t i = 0; i < BlockSize1; ++i) {
            writer.write(fill_one_packet(sn++));
        }

        CHECK(writer.alive());
        CHECK(dispatcher.source_size() == BlockSize1);
        CHECK(dispatcher.repair_size() == NumRepairPackets);

        mock_allocator.set_fail(true);
        CHECK(writer.resize(BlockSize2));

        for (size_t i = 0; i < BlockSize2; ++i) {
            writer.write(fill_one_packet(sn++));
        }

        CHECK(!writer.alive());
        CHECK(dispatcher.source_size() == BlockSize1);
        CHECK(dispatcher.repair_size() == NumRepairPackets);
    }
}

TEST(writer_reader, error_reader_resize_block) {
    enum { BlockSize1 = 50, BlockSize2 = 60 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        MockAllocator mock_allocator;

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool,
                      mock_allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        size_t sn = 0;

        // write first block
        CHECK(writer.resize(BlockSize1));
        for (size_t i = 0; i < BlockSize1; ++i) {
            writer.write(fill_one_packet(sn++));
        }

        // deliver first block
        dispatcher.push_written();

        // read first block
        for (size_t i = 0; i < BlockSize1; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        // write second block
        CHECK(writer.resize(BlockSize2));
        for (size_t i = 0; i < BlockSize2; ++i) {
            writer.write(fill_one_packet(sn++));
        }

        // deliver second block
        dispatcher.push_written();

        // configure allocator to return errors
        mock_allocator.set_fail(true);

        // reader should get an error from allocator when trying
        // to resize the block and shut down
        CHECK(!reader.read());
        CHECK(!reader.alive());
    }
}

TEST(writer_reader, error_reader_decode_packet) {
    enum { BlockSize1 = 50, BlockSize2 = 60 };

    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; n_scheme++) {
        config.scheme = Test_fec_schemes[n_scheme];

        MockAllocator mock_allocator;

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, mock_allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool,
                      allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        size_t sn = 0;

        // write first block
        CHECK(writer.resize(BlockSize1));
        for (size_t i = 0; i < BlockSize1; ++i) {
            writer.write(fill_one_packet(sn++));
        }

        // deliver first block
        dispatcher.push_written();

        // read first block
        for (size_t i = 0; i < BlockSize1; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, i);
            check_restored(p, false);
        }

        // lose one packet in second block
        dispatcher.reset();
        dispatcher.lose(10);

        // write second block
        CHECK(writer.resize(BlockSize2));
        for (size_t i = 0; i < BlockSize2; ++i) {
            writer.write(fill_one_packet(sn++));
        }

        // deliver second block
        dispatcher.push_written();

        // read second block packets before loss
        for (size_t i = 0; i < 10; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, BlockSize1 + i);
            check_restored(p, false);
        }

        // configure allocator to return errors
        mock_allocator.set_fail(true);

        // reader should get an error from allocator when trying
        // to repair lost packet and shut down
        CHECK(!reader.read());
        CHECK(!reader.alive());
    }
}

TEST(writer_reader, writer_oversized_block) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; ++n_scheme) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        CHECK(decoder.max_block_length() == encoder.max_block_length());
        CHECK(NumSourcePackets + NumRepairPackets < encoder.max_block_length());

        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer(),
                      repair_composer(), packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        // try to resize writer with an invalid value
        CHECK(!writer.resize(encoder.max_block_length() + 1));

        // ensure that the block size was not updated
        for (size_t n = 0; n < 10; ++n) {
            fill_all_packets(0);

            // write packets to dispatcher
            for (size_t i = 0; i < NumSourcePackets; ++i) {
                writer.write(source_packets[i]);
            }

            // deliver packets from dispatcher to reader
            dispatcher.push_written();

            CHECK(dispatcher.source_size() == NumSourcePackets);
            CHECK(dispatcher.repair_size() == NumRepairPackets);

            // read packets
            for (size_t i = 0; i < NumSourcePackets; ++i) {
                packet::PacketPtr p = reader.read();
                CHECK(p);

                check_audio_packet(p, i);
                check_restored(p, false);

                CHECK(p->fec()->source_block_length == NumSourcePackets);
                CHECK(p->fec()->block_length == NumSourcePackets + NumRepairPackets);
            }

            CHECK(reader.alive());
            CHECK(dispatcher.source_size() == 0);
            CHECK(dispatcher.repair_size() == 0);
        }
    }
}

TEST(writer_reader, reader_oversized_block) {
    for (size_t n_scheme = 0; n_scheme < Test_n_fec_schemes; ++n_scheme) {
        config.scheme = Test_fec_schemes[n_scheme];

        OFEncoder encoder(config, FECPayloadSize, allocator);
        OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

        CHECK(encoder.valid());
        CHECK(decoder.valid());

        CHECK(decoder.max_block_length() == encoder.max_block_length());
        CHECK((NumSourcePackets + NumRepairPackets) < encoder.max_block_length());

        packet::Queue queue;
        PacketDispatcher dispatcher(NumSourcePackets, NumRepairPackets);

        // We are going to violates source_block_length field of a FEC packet,
        // but Reed-Solomon does not allows us to set this field more than 255.
        // As a result LDPC composer is used.
        Writer writer(config, FECPayloadSize, encoder, queue, ldpc_source_composer,
                      ldpc_repair_composer, packet_pool, buffer_pool, allocator);

        Reader reader(config, decoder, dispatcher.source_reader(),
                      dispatcher.repair_reader(), rtp_parser, packet_pool, allocator);

        CHECK(writer.valid());
        CHECK(reader.valid());

        fill_all_packets(0);

        // encode packets and write to queue
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }

        // write packets from queue to dispatcher
        for (size_t i = 0; i < NumSourcePackets + NumRepairPackets; ++i) {
            packet::PacketPtr p = queue.read();
            CHECK(p);

            // update block size at the beginning of the block
            if (i == 0) {
                // violates: SBL + RBL <= MAX_NES (for source packets)
                p->fec()->source_block_length = encoder.max_block_length() + 1;
                // reset block length to ensure that our packet won't be dropped
                p->fec()->block_length = 0;
            }

            dispatcher.write(p);
        }

        // deliver packets from dispatcher to reader
        dispatcher.push_written();

        CHECK(dispatcher.source_size() == NumSourcePackets);
        CHECK(dispatcher.repair_size() == NumRepairPackets);

        // reader should get an error because maximum block size was exceeded
        CHECK(!reader.read());
        CHECK(!reader.alive());
    }
}

} // namespace fec
} // namespace roc
