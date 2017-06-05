/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/streamer.h"
#include "roc_core/scoped_ptr.h"

#include "test_packet_reader.h"

namespace roc {
namespace test {

using namespace audio;

using packet::timestamp_t;

namespace {

enum { ChNum = 1, ChMask = 0x3 };

enum { NumSamples = 20, NumPackets = 100, BufSz = NumSamples * NumPackets };

enum { Rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE };

} // namespace

TEST_GROUP(streamer) {
    TestPacketReader<NumPackets> reader;

    core::ScopedPtr<Streamer> streamer;

    void setup() {
        streamer.reset(new Streamer(reader, ChNum));
    }

    void add_packet(packet::timestamp_t timestamp, packet::sample_t value) {
        packet::IPacketPtr packet = new_audio_packet();

        packet::sample_t samples[NumSamples];

        for (size_t n = 0; n < NumSamples; n++) {
            samples[n] = value;
        }

        packet->rtp()->set_timestamp(timestamp);
        packet->audio()->configure(ChMask, NumSamples, Rate);
        packet->audio()->write_samples((1 << ChNum), 0, samples, NumSamples);

        reader.add(packet);
    }

    void expect_buffers(size_t num_buffers, size_t sz, packet::sample_t value) {
        read_buffers<BufSz>(*streamer, num_buffers, sz, value);
    }
};

TEST(streamer, one_packet_one_read) {
    add_packet(0, 0.333f);

    expect_buffers(1, NumSamples, 0.333f);
}

TEST(streamer, one_packet_multiple_reads) {
    add_packet(0, 0.333f);

    expect_buffers(NumSamples, 1, 0.333f);
}

TEST(streamer, multiple_packets_one_read) {
    for (size_t n = 0; n < NumPackets; n++) {
        add_packet(NumSamples * (timestamp_t)n, 0.333f);
    }

    expect_buffers(1, NumPackets * NumSamples, 0.333f);
}

TEST(streamer, multiple_packets_multiple_reads) {
    CHECK(NumSamples % 10 == 0);

    add_packet(NumSamples * 1, 0.333f);
    add_packet(NumSamples * 2, 0.444f);
    add_packet(NumSamples * 3, 0.555f);

    expect_buffers(10, NumSamples / 10, 0.333f);
    expect_buffers(10, NumSamples / 10, 0.444f);
    expect_buffers(10, NumSamples / 10, 0.555f);
}

TEST(streamer, timestamp_overflow) {
    timestamp_t ts2 = 0;
    timestamp_t ts1 = ts2 - NumSamples;
    timestamp_t ts3 = ts2 + NumSamples;

    add_packet(ts1, 0.333f);
    add_packet(ts2, 0.444f);
    add_packet(ts3, 0.555f);

    expect_buffers(NumSamples, 1, 0.333f);
    expect_buffers(NumSamples, 1, 0.444f);
    expect_buffers(NumSamples, 1, 0.555f);
}

TEST(streamer, drop_late_packets) {
    timestamp_t ts1 = NumSamples * 2;
    timestamp_t ts2 = NumSamples * 1;
    timestamp_t ts3 = NumSamples * 3;

    add_packet(ts1, 0.111f);
    add_packet(ts2, 0.222f);
    add_packet(ts3, 0.333f);

    expect_buffers(NumSamples, 1, 0.111f);
    expect_buffers(NumSamples, 1, 0.333f);
}

TEST(streamer, drop_late_packets_timestamp_overflow) {
    timestamp_t ts1 = 0;
    timestamp_t ts2 = ts1 - NumSamples;
    timestamp_t ts3 = ts1 + NumSamples;

    add_packet(ts1, 0.111f);
    add_packet(ts2, 0.222f);
    add_packet(ts3, 0.333f);

    expect_buffers(NumSamples, 1, 0.111f);
    expect_buffers(NumSamples, 1, 0.333f);
}

TEST(streamer, zeros_no_packets) {
    expect_buffers(1, NumSamples, 0);
}

TEST(streamer, zeros_no_next_packet) {
    add_packet(0, 0.333f);

    expect_buffers(1, NumSamples, 0.333f);
    expect_buffers(1, NumSamples, 0.000f);
}

TEST(streamer, zeros_between_packets) {
    add_packet(NumSamples * 1, 0.111f);
    add_packet(NumSamples * 3, 0.333f);

    expect_buffers(NumSamples, 1, 0.111f);
    expect_buffers(NumSamples, 1, 0.000f);
    expect_buffers(NumSamples, 1, 0.333f);
}

TEST(streamer, zeros_between_packets_timestamp_overflow) {
    timestamp_t ts2 = 0;
    timestamp_t ts1 = ts2 - NumSamples;
    timestamp_t ts3 = ts2 + NumSamples;

    add_packet(ts1, 0.111f);
    add_packet(ts3, 0.333f);

    expect_buffers(NumSamples, 1, 0.111f);
    expect_buffers(NumSamples, 1, 0.000f);
    expect_buffers(NumSamples, 1, 0.333f);
}

TEST(streamer, zeros_after_packet) {
    CHECK(NumSamples % 2 == 0);

    add_packet(0, 0.333f);

    ISampleBufferPtr buf1 = new_buffer<BufSz>(NumSamples / 2);
    ISampleBufferPtr buf2 = new_buffer<BufSz>(NumSamples);

    streamer->read(*buf1);
    streamer->read(*buf2);

    expect_data(buf1->data(), NumSamples / 2, 0.333f);
    expect_data(buf2->data(), NumSamples / 2, 0.333f);
    expect_data(buf2->data() + NumSamples / 2, NumSamples / 2, 0.000f);
}

TEST(streamer, packet_after_zeros) {
    expect_buffers(NumSamples, 1, 0.000f);

    add_packet(0, 0.111f);

    expect_buffers(NumSamples, 1, 0.111f);
}

TEST(streamer, overlapping_packets) {
    const size_t N = NumSamples;

    CHECK(N % 2 == 0);

    timestamp_t ts1 = 0;
    timestamp_t ts2 = N / 2;
    timestamp_t ts3 = N;

    add_packet(ts1, 0.111f);
    add_packet(ts2, 0.222f);
    add_packet(ts3, 0.333f);

    expect_buffers(N, 1, 0.111f);
    expect_buffers(N / 2, 1, 0.222f);
    expect_buffers(N / 2, 1, 0.333f);
}

} // namespace test
} // namespace roc
