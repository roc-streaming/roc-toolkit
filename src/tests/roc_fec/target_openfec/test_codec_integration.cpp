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

#include "roc_fec/ldpc_block_encoder.h"
#include "roc_fec/ldpc_block_decoder.h"

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

const double EPSILON = 0.0001;

typedef LDPC_BlockEncoder BlockEncoder;
typedef LDPC_BlockDecoder BlockDecoder;

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
            data_queue_.write(p);
        } else if (IFECPacket::Type) {
            fec_queue_.write(p);
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
        return data_queue_.size();
    }

    size_t get_fec_size() {
        return fec_queue_.size();
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

private:
    size_t packet_num_;

    PacketQueue data_queue_;
    PacketQueue fec_queue_;

    std::set<size_t> lost_packet_nums_;
};

} // namespace

TEST_GROUP(fec_codec_integration) {
    rtp::Composer composer;
    PacketDispatcher pckt_disp;

    IPacketPtr data_packets[N_DATA_PACKETS];

    void setup() {
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
        audio_packet->set_size(LEFT | RIGHT, N_SAMPLES);
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
    BlockEncoder block_encoder;
    BlockDecoder block_decoder;
    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }

    CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS);
    CHECK(pckt_disp.get_fec_size() == N_FEC_PACKETS);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i, N_DATA_PACKETS);
    }
}

TEST(fec_codec_integration, 1_loss) {
    BlockEncoder block_encoder;
    BlockDecoder block_decoder;
    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    pckt_disp.lose(11);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }

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

    BlockEncoder block_encoder;
    BlockDecoder block_decoder;
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

    BlockEncoder block_encoder;
    BlockDecoder block_decoder;
    rtp::Parser parser;

    Interleaver intrl(pckt_disp);

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    IPacketPtr many_packets[N_PACKETS];

    for (size_t i = 0; i < N_PACKETS; ++i) {
        many_packets[i] = fill_one_packet(i, N_PACKETS);
        encoder.write(many_packets[i]);
    }

    intrl.flush();

    for (size_t i = 0; i < N_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i, N_PACKETS);
    }
}

TEST(fec_codec_integration, decoding_when_multiple_blocks_in_queue) {
    enum { N_BLKS = 3 };

    BlockEncoder block_encoder;
    BlockDecoder block_decoder;
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

TEST(fec_codec_integration, decoding_late_packet)
{
    // 1. Fill all packets in block except one lost packet.
    // 2. Read first part of block till lost packet.
    // 3. Receive one missing packet.
    // 4. Read and check latter block part.

    BlockEncoder block_encoder;
    BlockDecoder block_decoder;
    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    fill_all_packets( 0, N_DATA_PACKETS);
    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        // Loosing packet #10
        if( i == 10)
            continue;
        encoder.write(data_packets[i]);
    }
    CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS-1);

    // Check 0-9 packets.
    for (size_t i = 0; i < 10; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i,
                           N_DATA_PACKETS);
    }

    // Receive packet #10
    encoder.write(data_packets[10]);

    for (size_t i = 10; i < N_DATA_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i,
                           N_DATA_PACKETS);
    }
}

IGNORE_TEST(fec_codec_integration, get_packets_before_marker_bit) {

    // 1. Fill second half of block and whole block with one loss after. So that there is 10-19 and 20-39 seqnums in packet queue.
    // 2. Check that we've got every packet including lost one.

    BlockEncoder block_encoder;
    BlockDecoder block_decoder;
    rtp::Parser parser;

    Encoder encoder(block_encoder, pckt_disp, composer);
    Decoder decoder(block_decoder, pckt_disp.get_data_reader(),
                    pckt_disp.get_fec_reader(), parser);

    fill_all_packets( 0, N_DATA_PACKETS * 2);
    for (size_t i = 10; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }

    fill_all_packets( N_DATA_PACKETS, N_DATA_PACKETS * 2);
    pckt_disp.lose(3);

    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        encoder.write(data_packets[i]);
    }

    for (size_t i = 10; i < N_DATA_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i,
                           N_DATA_PACKETS * 2);
    }
    // CHECK(pckt_disp.get_data_size() == N_DATA_PACKETS);
    for (size_t i = 0; i < N_DATA_PACKETS; ++i) {
        IPacketConstPtr p = decoder.read();
        CHECK(p);
        check_audio_packet(p, i + N_DATA_PACKETS,
                           N_DATA_PACKETS * 2);
    }
}

IGNORE_TEST(fec_codec_integration, repair_wrong_source_or_seqnum) {
}

} // namespace test
} // namespace roc
