/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>
#include <set>

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
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/sorted_queue.h"
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
fec::Composer<RSm8_PayloadID, Source, Footer> source_composer(&rtp_composer);
fec::Composer<RSm8_PayloadID, Repair, Header> repair_composer_inner(NULL);
rtp::Composer repair_composer(&repair_composer_inner);

// Divides packets from Encoder into two queues: source and repair packets,
// as needed for Decoder.
class PacketDispatcher : public packet::IWriter {
public:
    PacketDispatcher()
        : packet_num_(0)
        , source_queue_(0)
        , source_stock_(0)
        , repair_queue_(0)
        , repair_stock_(0) {
        reset();
    }

    virtual void write(const packet::PacketPtr& p) {
        if (lost_packet_nums_.find(packet_num_) != lost_packet_nums_.end()) {
            ++packet_num_;
            return;
        }

        if (p->flags() & packet::Packet::FlagAudio) {
            source_stock_.write(p);
        } else if (p->flags() & packet::Packet::FlagRepair) {
            repair_stock_.write(p);
        } else {
            FAIL("unexpected packet type");
        }

        if (++packet_num_ >= NumSourcePackets + NumRepairPackets) {
            packet_num_ = 0;
        }
    }

    packet::IReader& source_reader() {
        return source_queue_;
    }

    packet::IReader& repair_reader() {
        return repair_queue_;
    }

    size_t source_size() {
        return source_stock_.size() + source_queue_.size();
    }

    size_t repair_size() {
        return repair_stock_.size() + repair_queue_.size();
    }

    packet::PacketPtr repair_head() {
        return repair_queue_.head();
    }

    //! Clears both queues.
    void reset() {
        const size_t n_source_packets = source_queue_.size();
        const size_t n_repair_packets = repair_queue_.size();

        for (size_t i = 0; i < n_source_packets; ++i) {
            source_queue_.read();
        }

        for (size_t i = 0; i < n_repair_packets; ++i) {
            repair_queue_.read();
        }

        packet_num_ = 0;
        lost_packet_nums_.clear();
    }

    void lose(const size_t n) {
        lost_packet_nums_.insert(n);
    }

    void clear_losses() {
        lost_packet_nums_.clear();
    }

    void release_all() {
        while (source_stock_.head()) {
            source_queue_.write(source_stock_.read());
        }
        while (repair_stock_.head()) {
            repair_queue_.write(repair_stock_.read());
        }
    }

    bool pop_source() {
        packet::PacketPtr p;
        if (!(p = source_stock_.read())) {
            return false;
        }
        source_queue_.write(p);
        return true;
    }

private:
    size_t packet_num_;

    packet::SortedQueue source_queue_;
    packet::SortedQueue source_stock_;

    packet::SortedQueue repair_queue_;
    packet::SortedQueue repair_stock_;

    std::set<size_t> lost_packet_nums_;
};

} // namespace

TEST_GROUP(writer_reader) {
    packet::PacketPtr source_packets[NumSourcePackets];

    Config config;

    void setup() {
        config.codec = ReedSolomon8m;
        config.n_source_packets = NumSourcePackets;
        config.n_repair_packets = NumRepairPackets;

        fill_all_packets(0);
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

        CHECK(source_composer.prepare(*pp, bp, RTPPayloadSize));

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
};

TEST(writer_reader, read_write_lossless) {
    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
    }
    dispatcher.release_all();

    CHECK(dispatcher.source_size() == NumSourcePackets);
    CHECK(dispatcher.repair_size() == NumRepairPackets);

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        packet::PacketPtr p = reader.read();
        CHECK(p);
        check_audio_packet(p, i);
    }
}

TEST(writer_reader, 1_loss) {
    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    dispatcher.lose(11);

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
    }
    dispatcher.release_all();

    LONGS_EQUAL(NumSourcePackets - 1, dispatcher.source_size());
    LONGS_EQUAL(NumRepairPackets, dispatcher.repair_size());

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        packet::PacketPtr p = reader.read();
        CHECK(p);
        check_audio_packet(p, i);
    }
}

TEST(writer_reader, multiblocks_1_loss) {
    enum { NumBlocks = 40 };

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

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
        dispatcher.release_all();

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
        }

        dispatcher.reset();
    }
}

TEST(writer_reader, interleaver) {
    enum { NumPackets = NumSourcePackets * 30 };

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    packet::Interleaver intrlvr(dispatcher, allocator, 10);

    CHECK(intrlvr.valid());

    Writer writer(config, FECPayloadSize, encoder, intrlvr, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    packet::PacketPtr many_packets[NumPackets];

    for (size_t i = 0; i < NumPackets; ++i) {
        many_packets[i] = fill_one_packet(i);
        writer.write(many_packets[i]);
    }
    dispatcher.release_all();

    intrlvr.flush();

    for (size_t i = 0; i < NumPackets; ++i) {
        packet::PacketPtr p = reader.read();
        CHECK(p);
        check_audio_packet(p, i);
    }
}

TEST(writer_reader, decoding_when_multiple_blocks_in_queue) {
    enum { NumBlocks = 3 };

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    for (size_t block_num = 0; block_num < NumBlocks; ++block_num) {
        fill_all_packets(NumSourcePackets * block_num);

        for (size_t i = 0; i < NumSourcePackets; ++i) {
            writer.write(source_packets[i]);
        }
    }
    dispatcher.release_all();

    CHECK(dispatcher.source_size() == NumSourcePackets * NumBlocks);
    CHECK(dispatcher.repair_size() == NumRepairPackets * NumBlocks);

    for (size_t block_num = 0; block_num < NumBlocks; ++block_num) {
        for (size_t i = 0; i < NumSourcePackets; ++i) {
            packet::PacketPtr p = reader.read();
            CHECK(p);
            check_audio_packet(p, NumSourcePackets * block_num + i);
        }

        dispatcher.reset();
    }
}

IGNORE_TEST(writer_reader, decoding_late_packet) {
    // 1. Fill all packets in block except one lost packet.
    // 2. Read first part of block till lost packet.
    // 3. Receive one missing packet.
    // 4. Read and check latter block part.

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    fill_all_packets(0);
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        // Hold from #7 to #10
        if (i >= 7 && i <= 10) {
            continue;
        }
        writer.write(source_packets[i]);
    }
    dispatcher.release_all();
    CHECK(dispatcher.source_size() == NumSourcePackets - (10 - 7 + 1));

    // Check 0-7 packets.
    for (size_t i = 0; i < 7; ++i) {
        packet::PacketPtr p = reader.read();
    }
    // Receive packet #9 and #10
    writer.write(source_packets[9]);
    writer.write(source_packets[10]);
    dispatcher.pop_source();
    dispatcher.pop_source();

    for (size_t i = 9; i < NumSourcePackets; ++i) {
        packet::PacketPtr p = reader.read();
        CHECK(p);
        check_audio_packet(p, i);
        // Receive late packet that Reader have to throw away.
        if (i == 10) {
            writer.write(source_packets[7]);
            writer.write(source_packets[8]);
            dispatcher.pop_source();
            dispatcher.pop_source();
        }
    }
    LONGS_EQUAL(0, dispatcher.source_size());
}

TEST(writer_reader, get_packets_before_marker_bit) {
    // 1. Fill second half of block and whole block with one loss after, so that there
    //    is 10-19 and 20-39 seqnums in packet queue.
    // 2. Check that we've got every packet including lost one.

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    // Sending first block except first packet with marker bit.
    fill_all_packets(0);
    dispatcher.lose(0);
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
    }

    // Sending second block with start packet with marker bit.
    dispatcher.clear_losses();
    fill_all_packets(NumSourcePackets);
    // Lose one packe just to check if FEC is working correctly from the first block.
    dispatcher.lose(3);
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
    }
    dispatcher.release_all();

    // Receive every sent packet and the repaired one.
    for (size_t i = 1; i < NumSourcePackets * 2; ++i) {
        packet::PacketPtr p = reader.read();
        if (i < NumSourcePackets) {
            CHECK(!reader.started());
        } else {
            CHECK(reader.started());
        }
        check_audio_packet(p, i);
    }
    CHECK(dispatcher.source_size() == 0);
}

TEST(writer_reader, encode_packet_fields) {
    enum { NumBlocks = 3 };

    packet::source_t data_source = 555;

    for (size_t n = 0; n < 5; n++) {
        OFEncoder encoder(config, FECPayloadSize, allocator);

        CHECK(encoder.valid());

        PacketDispatcher dispatcher;

        Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                      repair_composer, packet_pool, buffer_pool, allocator);

        CHECK(writer.valid());

        packet::source_t fec_source = 0;
        packet::seqnum_t fec_seqnum = 0;
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
            dispatcher.release_all();

            if (block_num == 0) {
                fec_source = dispatcher.repair_head()->rtp()->source;
                fec_seqnum = dispatcher.repair_head()->rtp()->seqnum;
                fec_sbn = dispatcher.repair_head()->fec()->source_block_number;
            }

            CHECK(fec_source != data_source);

            for (size_t i = 0; i < NumSourcePackets; ++i) {
                const packet::PacketPtr p = dispatcher.source_reader().read();
                CHECK(p);

                LONGS_EQUAL(data_source, p->rtp()->source);

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

                LONGS_EQUAL(fec_source, p->rtp()->source);
                LONGS_EQUAL(fec_seqnum, p->rtp()->seqnum);

                const packet::FEC* fec = p->fec();
                CHECK(fec);

                LONGS_EQUAL(fec_sbn, fec->source_block_number);
                CHECK(fec->source_block_length == NumSourcePackets);
                UNSIGNED_LONGS_EQUAL(encoding_symbol_id, fec->encoding_symbol_id);

                fec_seqnum++;
                encoding_symbol_id++;
            }

            fec_sbn++;
        }

        dispatcher.reset();

        data_source = fec_source;
    }
}

TEST(writer_reader, decode_bad_source_id) {
    // Spoil source id in packet and lose it.
    // Check that decoder would shutdown.

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    fill_all_packets(0);

    dispatcher.lose(5); // should shutdown reader (bad source id)

    source_packets[5]->rtp()->source = source_packets[5]->rtp()->source + 1;

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
    }
    dispatcher.release_all();

    for (size_t i = 0; i < 5; ++i) {
        check_audio_packet(reader.read(), i);
        CHECK(reader.alive());
    }

    for (size_t i = 5; i < NumSourcePackets; ++i) {
        CHECK(!reader.read());
        CHECK(!reader.alive());
    }

    CHECK(dispatcher.source_size() == 0);
}

TEST(writer_reader, multitime_decode) {
    // 1. Lose two distant packets and hold every fec packets in first block,
    //    receive second full block.
    // 2. Detect first loss.
    // 3. Transmit fec packets.
    // 4. Check remaining data packets including lost one.

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    fill_all_packets(0);

    dispatcher.lose(5);
    dispatcher.lose(15);

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
        dispatcher.pop_source();
    }

    fill_all_packets(NumSourcePackets);
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
        dispatcher.pop_source();
    }

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        if (i != 5 && i != 15) {
            check_audio_packet(reader.read(), i);
            // The moment of truth.
        } else if (i == 15) {
            // Get FEC packets. Reader must try to decode once more.
            dispatcher.release_all();
            check_audio_packet(reader.read(), i);
        }
    }
    for (size_t i = 0; i < NumSourcePackets; ++i) {
        check_audio_packet(reader.read(), i + NumSourcePackets);
    }

    LONGS_EQUAL(0, dispatcher.source_size());
}

TEST(writer_reader, delayed_packets) {
    // 1. Fill first half of block.
    // 2. Check that we receive only this first 10 packets.
    // 3. Send remaining packets.
    // 4. Receive and check it.

    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

    Reader reader(config, decoder, dispatcher.source_reader(), dispatcher.repair_reader(),
                  rtp_parser, packet_pool, allocator);

    CHECK(writer.valid());
    CHECK(reader.valid());

    fill_all_packets(0);

    for (size_t i = 0; i < NumSourcePackets; ++i) {
        writer.write(source_packets[i]);
    }
    for (size_t i = 0; i < 10; ++i) {
        dispatcher.pop_source();
    }

    for (size_t i = 0; i < 10; ++i) {
        check_audio_packet(reader.read(), i);
    }
    CHECK(!reader.read());
    dispatcher.release_all();
    for (size_t i = 10; i < NumSourcePackets; ++i) {
        check_audio_packet(reader.read(), i);
    }
}

TEST(writer_reader, drop_outdated_block) {
    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

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

    dispatcher.release_all();

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

TEST(writer_reader, repaired_block_numbering) {
    OFEncoder encoder(config, FECPayloadSize, allocator);
    OFDecoder decoder(config, FECPayloadSize, buffer_pool, allocator);

    CHECK(encoder.valid());
    CHECK(decoder.valid());

    PacketDispatcher dispatcher;

    Writer writer(config, FECPayloadSize, encoder, dispatcher, source_composer,
                  repair_composer, packet_pool, buffer_pool, allocator);

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

    dispatcher.release_all();

    // Read first block.
    const packet::PacketPtr first_packet = reader.read();
    CHECK(first_packet);

    const packet::blknum_t sbn = first_packet->fec()->source_block_number;

    for (size_t n = 1; n < NumSourcePackets; ++n) {
        const packet::PacketPtr p = reader.read();
        CHECK(p);

        if (n == lost_packet_n) {
            check_audio_packet(p, n);
        } else {
            CHECK(p->fec()->source_block_number == sbn);
        }
    }

    // Read second block.
    for (size_t n = 0; n < NumSourcePackets; ++n) {
        const packet::PacketPtr p = reader.read();
        CHECK(p);

        CHECK(p->fec()->source_block_number == sbn + 1);
    }
}

} // namespace fec
} // namespace roc
