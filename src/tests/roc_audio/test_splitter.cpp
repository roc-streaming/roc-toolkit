/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_config/config.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/log.h"
#include "roc_rtp/composer.h"
#include "roc_packet/iaudio_packet.h"
#include "roc_audio/splitter.h"

#include "test_packet_writer.h"
#include "test_helpers.h"

namespace roc {
namespace test {

using namespace audio;
using namespace packet;

namespace {

const channel_t CH_0 = 0, CH_1 = 1;

const channel_mask_t TEST_CHANNELS = (1 << CH_0) | (1 << CH_1);

const size_t TEST_N_PACKETS = 100;
const size_t TEST_N_BUFS = 20;

const size_t TEST_N_CH = 2;
const size_t TEST_N_SAMPLES = TEST_N_BUFS * 13;

const long TEST_MAX_VAL = 100;

} // namespace

TEST_GROUP(splitter) {
    TestPacketWriter<TEST_N_PACKETS> writer;
    rtp::Composer composer;

    core::ScopedPtr<Splitter> splitter;

    seqnum_t sn;
    timestamp_t ts;
    size_t pkt_num;
    size_t sample_num;

    void setup() {
        splitter.reset(new Splitter(writer, composer, TEST_N_SAMPLES, TEST_CHANNELS));
        pkt_num = 0;
    }

    void teardown() {
        LONGS_EQUAL(writer.num_packets(), pkt_num);
    }

    ISampleBufferConstPtr make_buffer(size_t num, size_t n_samples) {
        ISampleBufferPtr buff = new_buffer<TEST_N_SAMPLES * TEST_N_CH * TEST_N_PACKETS>(
            n_samples * TEST_N_CH);

        for (size_t n = 0; n < buff->size(); n++) {
            buff->data()[n] =
                sample_t((num * n_samples * TEST_N_CH + n) % TEST_MAX_VAL) / TEST_MAX_VAL;
        }

        return buff;
    }

    IAudioPacketPtr get_packet(size_t n) {
        IPacketPtr packet = writer.packet(n);

        CHECK(packet);
        CHECK(packet->type() == IAudioPacket::Type);

        roc_panic_if_not(packet->type() == IAudioPacket::Type);

        return static_cast<IAudioPacket*>(packet.get());
    }

    void read_packet() {
        IAudioPacketPtr packet = get_packet(pkt_num);

        LONGS_EQUAL(TEST_CHANNELS, packet->channels());
        LONGS_EQUAL(TEST_N_SAMPLES, packet->num_samples());
        CHECK(!packet->marker());

        if (pkt_num == 0) {
            sn = packet->seqnum();
            ts = packet->timestamp();
        } else {
            LONGS_EQUAL(sn, packet->seqnum());
            LONGS_EQUAL(ts, packet->timestamp());
        }

        sample_t samples[TEST_N_SAMPLES * TEST_N_CH] = {};

        LONGS_EQUAL(TEST_N_SAMPLES,
                    packet->read_samples(TEST_CHANNELS, 0, samples, TEST_N_SAMPLES));

        for (size_t n = 0; n < TEST_N_SAMPLES * TEST_N_CH; n++) {
            DOUBLES_EQUAL(sample_t(sample_num % TEST_MAX_VAL) / TEST_MAX_VAL, //
                          samples[n],                                         //
                          0.0001);
            sample_num++;
        }

        sn++;
        ts += TEST_N_SAMPLES;
        pkt_num++;
    }
};

TEST(splitter, one_buffer_one_packet) {
    for (size_t bn = 0; bn < TEST_N_BUFS; bn++) {
        LONGS_EQUAL(bn, writer.num_packets());

        ISampleBufferConstPtr buf = make_buffer(bn, TEST_N_SAMPLES);

        splitter->write(*buf);

        LONGS_EQUAL(bn + 1, writer.num_packets());

        read_packet();
    }
}

TEST(splitter, one_buffer_multiple_packets) {
    ISampleBufferConstPtr buf = make_buffer(0, TEST_N_SAMPLES * TEST_N_PACKETS);

    splitter->write(*buf);

    LONGS_EQUAL(TEST_N_PACKETS, writer.num_packets());

    for (size_t pn = 0; pn < TEST_N_PACKETS; pn++) {
        read_packet();
    }
}

TEST(splitter, multiple_buffers_one_packet) {
    CHECK(TEST_N_SAMPLES % TEST_N_BUFS == 0);

    for (size_t pn = 0; pn < TEST_N_PACKETS; pn++) {
        //
        for (size_t bn = 0; bn < TEST_N_BUFS; bn++) {
            LONGS_EQUAL(pn, writer.num_packets());

            ISampleBufferConstPtr buf =
                make_buffer(pn * TEST_N_BUFS + bn, TEST_N_SAMPLES / TEST_N_BUFS);

            splitter->write(*buf);
        }

        LONGS_EQUAL(pn + 1, writer.num_packets());

        read_packet();
    }
}

TEST(splitter, multiple_buffers_multiple_packets) {
    const size_t n_samples = (TEST_N_SAMPLES - 1);

    const size_t n_packets = (n_samples * TEST_N_BUFS / TEST_N_SAMPLES);

    for (size_t bn = 0; bn < TEST_N_BUFS; bn++) {
        //
        ISampleBufferConstPtr buf = make_buffer(bn, n_samples);

        splitter->write(*buf);
    }

    LONGS_EQUAL(n_packets, writer.num_packets());

    for (size_t pn = 0; pn < n_packets; pn++) {
        read_packet();
    }
}

} // namespace test
} // namespace roc
