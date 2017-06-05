/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/splitter.h"
#include "roc_config/config.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/ipacket.h"
#include "roc_rtp/composer.h"

#include "test_helpers.h"
#include "test_packet_writer.h"

namespace roc {
namespace test {

using namespace audio;
using namespace packet;

namespace {

enum { NumCh = 2, ChMask = 0x3 };

enum { NumPackets = 100, NumBufs = 20, NumSamples = NumBufs * 13 };

enum { MaxVal = 100 };

} // namespace

TEST_GROUP(splitter) {
    TestPacketWriter<NumPackets> writer;
    rtp::Composer composer;

    core::ScopedPtr<Splitter> splitter;

    source_t src;
    seqnum_t sn;
    timestamp_t ts;
    size_t pkt_num;
    size_t sample_num;

    void setup() {
        splitter.reset(new Splitter(writer, composer, NumSamples, ChMask));
        pkt_num = 0;
        sample_num = 0;
    }

    void teardown() {
        LONGS_EQUAL(writer.num_packets(), pkt_num);
    }

    ISampleBufferConstPtr make_buffer(size_t num, size_t n_samples, size_t n_trunc = 0) {
        ISampleBufferPtr buff =
            new_buffer<NumSamples * NumCh * NumPackets>((n_samples - n_trunc) * NumCh);

        for (size_t n = 0; n < buff->size(); n++) {
            buff->data()[n] = sample_t((num * n_samples * NumCh + n) % MaxVal) / MaxVal;
        }

        return buff;
    }

    IPacketPtr get_packet(size_t n) {
        IPacketPtr packet = writer.packet(n);

        CHECK(packet);
        CHECK(packet->audio());

        return packet;
    }

    void read_packet(size_t n_pad = 0) {
        IPacketPtr packet = get_packet(pkt_num);

        LONGS_EQUAL(ChMask, packet->audio()->channels());
        LONGS_EQUAL(NumSamples, packet->audio()->num_samples());
        CHECK(!packet->rtp()->marker());

        if (pkt_num == 0) {
            src = packet->rtp()->source();
            sn = packet->rtp()->seqnum();
            ts = packet->rtp()->timestamp();
        } else {
            LONGS_EQUAL(src, packet->rtp()->source());
            LONGS_EQUAL(sn, packet->rtp()->seqnum());
            LONGS_EQUAL(ts, packet->rtp()->timestamp());
        }

        sample_t samples[NumSamples * NumCh] = {};
        size_t pos = 0;

        LONGS_EQUAL(NumSamples,
                    packet->audio()->read_samples(ChMask, 0, samples, NumSamples));

        for (; pos < (NumSamples - n_pad) * NumCh; pos++) {
            DOUBLES_EQUAL(sample_t(sample_num % MaxVal) / MaxVal, samples[pos], 0.0001);
            sample_num++;
        }

        for (; pos < NumSamples * NumCh; pos++) {
            DOUBLES_EQUAL(0, samples[pos], 0);
            sample_num++;
        }

        sn++;
        ts += NumSamples;
        pkt_num++;
    }
};

TEST(splitter, one_buffer_one_packet) {
    for (size_t bn = 0; bn < NumBufs; bn++) {
        LONGS_EQUAL(bn, writer.num_packets());

        ISampleBufferConstPtr buf = make_buffer(bn, NumSamples);

        splitter->write(*buf);

        LONGS_EQUAL(bn + 1, writer.num_packets());

        read_packet();
    }
}

TEST(splitter, one_buffer_multiple_packets) {
    ISampleBufferConstPtr buf = make_buffer(0, NumSamples * NumPackets);

    splitter->write(*buf);

    LONGS_EQUAL(NumPackets, writer.num_packets());

    for (size_t pn = 0; pn < NumPackets; pn++) {
        read_packet();
    }
}

TEST(splitter, multiple_buffers_one_packet) {
    CHECK(NumSamples % NumBufs == 0);

    for (size_t pn = 0; pn < NumPackets; pn++) {
        for (size_t bn = 0; bn < NumBufs; bn++) {
            LONGS_EQUAL(pn, writer.num_packets());

            ISampleBufferConstPtr buf =
                make_buffer(pn * NumBufs + bn, NumSamples / NumBufs);

            splitter->write(*buf);
        }

        LONGS_EQUAL(pn + 1, writer.num_packets());

        read_packet();
    }
}

TEST(splitter, multiple_buffers_multiple_packets) {
    const size_t n_samples = (NumSamples - 1);

    const size_t n_packets = (n_samples * NumBufs / NumSamples);

    for (size_t bn = 0; bn < NumBufs; bn++) {
        ISampleBufferConstPtr buf = make_buffer(bn, n_samples);
        splitter->write(*buf);
    }

    LONGS_EQUAL(n_packets, writer.num_packets());

    for (size_t pn = 0; pn < n_packets; pn++) {
        read_packet();
    }
}

TEST(splitter, flush) {
    enum { Padding = 10 };

    splitter->write(*make_buffer(0, NumSamples));
    splitter->write(*make_buffer(1, NumSamples));
    splitter->write(*make_buffer(2, NumSamples, Padding));

    LONGS_EQUAL(2, writer.num_packets());

    read_packet();
    read_packet();

    splitter->flush();

    LONGS_EQUAL(3, writer.num_packets());

    read_packet(Padding);
}

} // namespace test
} // namespace roc
