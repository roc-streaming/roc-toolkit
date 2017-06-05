/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include <set>

#include "roc_core/log.h"

#include "roc_fec/encoder.h"
#include "roc_fec/decoder.h"

#include "roc_fec/of_block_encoder.h"
#include "roc_fec/of_block_decoder.h"

#include "roc_packet/iaudio_packet.h"
#include "roc_packet/packet_queue.h"
#include "roc_packet/interleaver.h"

#include "roc_rtp/parser.h"
#include "roc_rtp/composer.h"

#include "roc_config/config.h"

namespace roc {
namespace test {

using namespace fec;
using namespace packet;

namespace {

const size_t N_DATA_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_DATA_PACKETS;
const size_t N_FEC_PACKETS = ROC_CONFIG_DEFAULT_FEC_BLOCK_REDUNDANT_PACKETS;

const size_t N_SAMPLES = ROC_CONFIG_DEFAULT_PACKET_SAMPLES;
const size_t N_CH = 2;

const int LEFT = (1 << 0);
const int RIGHT = (1 << 1);

const size_t RATE = ROC_CONFIG_DEFAULT_SAMPLE_RATE;

const double EPSILON = 0.0001;

typedef OFBlockEncoder BlockEncoder;
typedef OFBlockDecoder BlockDecoder;

// Divides packets from Encoder into two queues: data and fec packets,
// as needed for Decoder.
class PacketDispatcher : public IPacketWriter {
public:
    PacketDispatcher()
        : packet_num_(0) {
        reset();
    }

    virtual ~PacketDispatcher() {
    }

    virtual void write(const IPacketPtr& p) {
        if (lost_packet_nums_.find(packet_num_) != lost_packet_nums_.end()) {
            ++packet_num_;
            return;
        }

        if (p->type() == IAudioPacket::Type) {
            data_stock_.write(p);
        } else if (IFECPacket::Type) {
            fec_stock_.write(p);
        }

        if (++packet_num_ >= N_DATA_PACKETS + N_FEC_PACKETS) {
            packet_num_ = 0;
        }
    }

    IPacketReader& get_data_reader() {
        return data_queue_;
    }

    IPacketReader& get_fec_reader() {
        return fec_queue_;
    }

    size_t get_data_size() {
        return data_stock_.size() + data_queue_.size();
    }

    size_t get_fec_size() {
        return fec_stock_.size() + fec_queue_.size();
    }

    IPacketConstPtr get_fec_head() {
        return fec_queue_.head();
    }

    //! Clears both queues.
    void reset() {
        const size_t data_packets_n = data_queue_.size();
        const size_t fec_packets_n = fec_queue_.size();

        for (size_t i = 0; i < data_packets_n; ++i) {
            data_queue_.read();
        }

        for (size_t i = 0; i < fec_packets_n; ++i) {
            fec_queue_.read();
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
        while (data_stock_.head()) {
            data_queue_.write(data_stock_.read());
        }
        while (fec_stock_.head()) {
            fec_queue_.write(fec_stock_.read());
        }
    }

    bool pop_data() {
        IPacketConstPtr p;
        if (!(p = data_stock_.read())) {
            return false;
        }
        data_queue_.write(p);
        return true;
    }

private:
    size_t packet_num_;

    PacketQueue data_queue_;
    PacketQueue data_stock_;

    PacketQueue fec_queue_;
    PacketQueue fec_stock_;

    std::set<size_t> lost_packet_nums_;
};

} // namespace

TEST_GROUP(fec_codec_integration) {
    rtp::Composer composer;
    PacketDispatcher pckt_disp;

    IPacketPtr data_packets[N_DATA_PACKETS];

    FECConfig fec_conf;

    void setup() {
        fec_conf.type = ReedSolomon2m;
        fec_conf.n_source_packets = N_DATA_PACKETS;
        fec_conf.n_repair_packets = N_FEC_PACKETS;

        fill_all_packets(0, N_DATA_PACKETS);
    }

    void fill_all_packets(size_t sn, size_t n_pkts) {
        for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
            data_packets[i] = fill_one_packet(sn + i, n_pkts);
        }
    }

    IPacketPtr fill_one_packet(size_t sn, size_t n_pkts) {
        IPacketPtr packet = composer.compose(IAudioPacket::Type);
        CHECK(packet);

        sample_t samples[N_SAMPLES * N_CH] = {};

        for (size_t n = 0; n < N_SAMPLES * N_CH; n += N_CH) {
            sample_t s =
                sample_t(N_SAMPLES * N_CH * sn + n) / (N_SAMPLES * N_CH * n_pkts);

            samples[n + 0] = +s;
            samples[n + 1] = -s;
        }

        IAudioPacket* audio_packet = (IAudioPacket*)packet.get();
        audio_packet->set_seqnum((seqnum_t)sn);
        audio_packet->set_size(LEFT | RIGHT, N_SAMPLES, RATE);
        audio_packet->write_samples(LEFT | RIGHT, 0, samples, N_SAMPLES);

        return packet;
    }

    void check_audio_packet(IPacketConstPtr packet, size_t sn, size_t n_pkts) {
        CHECK(packet);
        const IAudioPacket* audio_packet = (const IAudioPacket*)packet.get();

        sample_t left[N_SAMPLES] = {};
        sample_t right[N_SAMPLES] = {};

        LONGS_EQUAL((seqnum_t)sn, audio_packet->seqnum());

        CHECK(audio_packet->num_samples() == N_SAMPLES);
        CHECK(audio_packet->channels() == (LEFT | RIGHT));

        CHECK(audio_packet->read_samples(LEFT, 0, left, N_SAMPLES) == N_SAMPLES);
        CHECK(audio_packet->read_samples(RIGHT, 0, right, N_SAMPLES) == N_SAMPLES);

        for (size_t n = 0; n < N_SAMPLES; ++n) {
            sample_t s =
                sample_t(N_SAMPLES * N_CH * sn + n * N_CH) / (N_SAMPLES * N_CH * n_pkts);

            DOUBLES_EQUAL(+s, left[n], EPSILON);
            DOUBLES_EQUAL(-s, right[n], EPSILON);
        }
    }
};

TEST(fec_codec_integration, encode) {
    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);
    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }
    pckt_disp.release_all();

    CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS);
    CHECK(pckt_disp.get_fec_size() == N_FEC_PACKETS);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i, N_DATA_PACKETS);
    }
}

TEST(fec_codec_integration, 1_loss) {
    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    pckt_disp.lose(11);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }
    pckt_disp.release_all();

    LONGS_EQUAL(N_DATA_PACKETS - 1, pckt_disp.get_data_size());
    LONGS_EQUAL(N_FEC_PACKETS, pckt_disp.get_fec_size());

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i, N_DATA_PACKETS);
    }
}

TEST(fec_codec_integration, multiblocks_1_loss) {
    enum { N_BLKS = 40 };

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    for (size_t block_num = 0; block_num < N_BLKS; ++block_num) {
        size_t lost_sq = size_t(-1);
        if (block_num != 5 && block_num != 21 && block_num != 22) {
            lost_sq = (block_num + 1) % (N_DATA_PACKETS + N_FEC_PACKETS);
            pckt_disp.lose(lost_sq);
        }

        fill_all_packets(N_DATA_PACKETS * block_num, N_DATA_PACKETS * N_BLKS);

        for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
            encoder.write(data_packets[i]);
        }
        pckt_disp.release_all();

        if (lost_sq == size_t(-1)) {
            CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS);
            CHECK(pckt_disp.get_fec_size() == N_FEC_PACKETS);
        } else if (lost_sq < N_DATA_PACKETS) {
            CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS - 1);
            CHECK(pckt_disp.get_fec_size() == N_FEC_PACKETS);
        } else {
            CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS);
            CHECK(pckt_disp.get_fec_size() == N_FEC_PACKETS - 1);
        }

        for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
            IPacketConstPtr p = decoder.read();
            CHECK(p);
            check_audio_packet(p, N_DATA_PACKETS * block_num + i,
                               N_DATA_PACKETS * N_BLKS);
        }

        pckt_disp.reset();
    }
}

TEST(fec_codec_integration, interleaver) {
    enum { N_PACKETS = N_DATA_PACKETS * 30 };

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Interleaver intrl(pckt_disp, 10);

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    IPacketPtr many_packets[N_PACKETS];

    for (size_t i = 0; i < N_PACKETS; ++i) {
        many_packets[i] = fill_one_packet(i, N_PACKETS);
        encoder.write(many_packets[i]);
    }
    pckt_disp.release_all();

    intrl.flush();

    for (size_t i = 0; i < N_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i, N_PACKETS);
    }
}

TEST(fec_codec_integration, decoding_when_multiple_blocks_in_queue) {
    enum { N_BLKS = 3 };

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    for (size_t block_num = 0; block_num < N_BLKS; ++block_num) {
        fill_all_packets(N_DATA_PACKETS * block_num, N_DATA_PACKETS * N_BLKS);

        for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
            encoder.write(data_packets[i]);
        }
    }
    pckt_disp.release_all();

    CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS * N_BLKS);
    CHECK(pckt_disp.get_fec_size() == N_FEC_PACKETS * N_BLKS);

    for (size_t block_num = 0; block_num < N_BLKS; ++block_num) {
        for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
            IPacketConstPtr p = decoder.read();
            CHECK(p);
            check_audio_packet(p, N_DATA_PACKETS * block_num + i,
                               N_DATA_PACKETS * N_BLKS);
        }

        pckt_disp.reset();
    }
}

TEST(fec_codec_integration, decoding_late_packet) {
    // 1. Fill all packets in block except one lost packet.
    // 2. Read first part of block till lost packet.
    // 3. Receive one missing packet.
    // 4. Read and check latter block part.

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    fill_all_packets(0, N_DATA_PACKETS);
    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        // Hold from #7 to #10
        if (i >= 7 && i <= 10) {
            continue;
        }
        encoder.write(data_packets[i]);
    }
    pckt_disp.release_all();
    CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS - (10 - 7 + 1));

    // Check 0-7 packets.
    for (size_t i = 0; i < 7; ++i) {
        IPacketConstPtr p = decoder.read();
    }
    // Receive packet #9 and #10
    encoder.write(data_packets[9]);
    encoder.write(data_packets[10]);
    pckt_disp.pop_data();
    pckt_disp.pop_data();

    for (size_t i = 9; i < N_DATA_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i, N_DATA_PACKETS);
        // Receive late packet that Decoder have to throw away.
        if (i == 10) {
            encoder.write(data_packets[7]);
            encoder.write(data_packets[8]);
            pckt_disp.pop_data();
            pckt_disp.pop_data();
        }
    }
    LONGS_EQUAL(0, pckt_disp.get_data_size());
}

TEST(fec_codec_integration, get_packets_before_marker_bit) {
    // 1. Fill second half of block and whole block with one loss after, so that there
    //    is 10-19 and 20-39 seqnums in packet queue.
    // 2. Check that we've got every packet including lost one.

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    // Sending first block except first packet with marker bit.
    fill_all_packets(0, N_DATA_PACKETS * 2);
    pckt_disp.lose(0);
    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }

    // Sending second block with start packet with marker bit.
    pckt_disp.clear_losses();
    fill_all_packets(N_DATA_PACKETS, N_DATA_PACKETS * 2);
    // Loose one packe just to check if FEC is working correctly from the first block.
    pckt_disp.lose(3);
    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }
    pckt_disp.release_all();

    // Receive every sent packet and the repaired one.
    for (size_t i = 1; i < N_DATA_PACKETS * 2; ++i) {
        IPacketConstPtr p = decoder.read();
        if (i < N_DATA_PACKETS) {
            CHECK(!decoder.is_started());
        } else {
            CHECK(decoder.is_started());
        }
        check_audio_packet(p, i, N_DATA_PACKETS * 2);
    }
    CHECK(pckt_disp.get_data_size() == 0);
}

TEST(fec_codec_integration, encode_source_id_and_seqnum) {
    enum { N_BLKS = 3 };

    source_t data_source = 555;

    for (size_t n = 0; n < 5; n++) {
        BlockEncoder block_encoder(fec_conf);
        Encoder encoder(block_encoder, pckt_disp, composer);

        source_t fec_source = 0;
        seqnum_t fec_seqnum = 0;

        for (size_t block_num = 0; block_num < N_BLKS; ++block_num) {
            fill_all_packets(N_DATA_PACKETS * block_num, N_DATA_PACKETS * N_BLKS);

            for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
                data_packets[i]->set_source(data_source);
            }

            for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
                encoder.write(data_packets[i]);
            }
            pckt_disp.release_all();

            if (block_num == 0) {
                fec_source = pckt_disp.get_fec_head()->source();
                fec_seqnum = pckt_disp.get_fec_head()->seqnum();
            }

            CHECK(fec_source != data_source);

            for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
                LONGS_EQUAL(data_source, pckt_disp.get_data_reader().read()->source());
            }

            for (size_t i = 0; i < N_FEC_PACKETS; ++i) {
                IPacketConstPtr p = pckt_disp.get_fec_reader().read();
                LONGS_EQUAL(fec_source, p->source());
                LONGS_EQUAL(fec_seqnum, p->seqnum());
                fec_seqnum++;
            }
        }

        pckt_disp.reset();

        data_source = fec_source;
    }
}

TEST(fec_codec_integration, decode_bad_seqnum) {
    // Spoil seqnum in packet and lose it.
    // Check that decoder wouldn't restore it.

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    fill_all_packets(0, N_DATA_PACKETS);

    pckt_disp.lose(9);  // should be rejected (bad seqnum)
    pckt_disp.lose(14); // should be rapaired

    data_packets[9]->set_seqnum(data_packets[9]->seqnum() + 1);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }
    pckt_disp.release_all();

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        if (i != 9) {
            check_audio_packet(decoder.read(), i, N_DATA_PACKETS);
        }
    }

    CHECK(pckt_disp.get_data_size() == 0);
}

TEST(fec_codec_integration, decode_bad_source_id) {
    // Spoil source id in packet and lose it.
    // Check that decoder would shutdown.

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    fill_all_packets(0, N_DATA_PACKETS);

    pckt_disp.lose(5); // should shutdown decoder (bad source id)

    data_packets[5]->set_source(data_packets[5]->source() + 1);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }
    pckt_disp.release_all();

    for (size_t i = 0; i < 5; ++i) {
        check_audio_packet(decoder.read(), i, N_DATA_PACKETS);
        CHECK(decoder.is_alive());
    }

    for (size_t i = 5; i < N_DATA_PACKETS; ++i) {
        CHECK(!decoder.read());
        CHECK(!decoder.is_alive());
    }

    CHECK(pckt_disp.get_data_size() == 0);
}

TEST(fec_codec_integration, multitime_decode) {
    // 1. Loose two distant packets and hold every fec packets in first block, receive
    // second full block.
    // 2. Detect first loss.
    // 3. Transmit fec packets.
    // 4. Check remaining data packets including lost one.

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    fill_all_packets(0, N_DATA_PACKETS * 2);

    pckt_disp.lose(5);
    pckt_disp.lose(15);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
        pckt_disp.pop_data();
    }

    fill_all_packets(N_DATA_PACKETS, N_DATA_PACKETS * 2);
    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
        pckt_disp.pop_data();
    }

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        if (i != 5 && i != 15) {
            check_audio_packet(decoder.read(), i, N_DATA_PACKETS * 2);
            // The moment of truth.
        } else if (i == 15) {
            // Get FEC packets. Decoder must try to decode once more.
            pckt_disp.release_all();
            check_audio_packet(decoder.read(), i, N_DATA_PACKETS * 2);
        }
    }
    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        check_audio_packet(decoder.read(), i + N_DATA_PACKETS, N_DATA_PACKETS * 2);
    }

    LONGS_EQUAL(0, pckt_disp.get_data_size());
}

TEST(fec_codec_integration, delayed_packets) {
    // 1. Fill first half of block.
    // 2. Check that we receive only this first 10 packets.
    // 3. Send remaining packets.
    // 4. Receive and check it.

    BlockEncoder block_encoder(fec_conf);
    BlockDecoder block_decoder(fec_conf);

    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    fill_all_packets(0, N_DATA_PACKETS);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }
    for (size_t i = 0; i < 10; ++i) {
        pckt_disp.pop_data();
    }

    for (size_t i = 0; i < 10; ++i) {
        check_audio_packet(decoder.read(), i, N_DATA_PACKETS);
    }
    CHECK(!decoder.read());
    pckt_disp.release_all();
    for (size_t i = 10; i < N_DATA_PACKETS; ++i) {
        check_audio_packet(decoder.read(), i, N_DATA_PACKETS);
    }
}

} // namespace test
} // namespace roc
