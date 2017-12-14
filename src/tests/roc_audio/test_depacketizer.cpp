/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/depacketizer.h"
#include "roc_audio/idecoder.h"
#include "roc_audio/iencoder.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/pcm_decoder.h"
#include "roc_rtp/pcm_encoder.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxBufSize = 5000,

    SamplesPerPacket = 200,
    NumCh = 2,
    ChMask = 0x3
};

core::HeapAllocator allocator;
core::BufferPool<sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, 1);
packet::PacketPool packet_pool(allocator, 1);

rtp::Composer rtp_composer(NULL);

rtp::PCMEncoder<int16_t, NumCh> pcm_encoder;
rtp::PCMDecoder<int16_t, NumCh> pcm_decoder;

} // namespace

TEST_GROUP(depacketizer) {
    packet::PacketPtr new_packet(packet::timestamp_t ts, sample_t value) {
        packet::PacketPtr pp = new(packet_pool) packet::Packet(packet_pool);
        CHECK(pp);

        core::Slice<uint8_t> bp =
            new (byte_buffer_pool) core::Buffer<uint8_t>(byte_buffer_pool);
        CHECK(bp);

        CHECK(rtp_composer.prepare(*pp, bp, pcm_encoder.payload_size(SamplesPerPacket)));

        pp->set_data(bp);

        pp->rtp()->timestamp = ts;
        pp->rtp()->duration = SamplesPerPacket;

        sample_t samples[SamplesPerPacket * NumCh];
        for (size_t n = 0; n < SamplesPerPacket * NumCh; n++) {
            samples[n] = value;
        }

        UNSIGNED_LONGS_EQUAL(
            SamplesPerPacket,
            pcm_encoder.write_samples(*pp, 0, samples, SamplesPerPacket, ChMask));

        CHECK(rtp_composer.compose(*pp));

        return pp;
    }

    Frame new_frame(size_t sz) {
        core::Slice<sample_t> samples =
            new (sample_buffer_pool) core::Buffer<sample_t>(sample_buffer_pool);
        samples.resize(sz * NumCh);
        Frame frame(samples);
        return frame;
    }

    void expect_values(const sample_t* samples, size_t num_samples, sample_t value) {
        for (size_t n = 0; n < num_samples; n++) {
            DOUBLES_EQUAL(value, samples[n], 0.0001);
        }
    }

    void expect_output(Depacketizer& depacketizer, size_t sz, sample_t value) {
        Frame frame = new_frame(sz);
        depacketizer.read(frame);

        UNSIGNED_LONGS_EQUAL(sz * NumCh, frame.samples().size());
        expect_values(frame.samples().data(), sz * NumCh, value);
    }
};

TEST(depacketizer, one_packet_one_read) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(0, 0.11f));

    expect_output(dp, SamplesPerPacket, 0.11f);
}

TEST(depacketizer, one_packet_multiple_reads) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(0, 0.11f));

    for (size_t n = 0; n < SamplesPerPacket; n++) {
        expect_output(dp, 1, 0.11f);
    }
}

TEST(depacketizer, multiple_packets_one_read) {
    enum { NumPackets = 10 };

    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    for (packet::timestamp_t n = 0; n < NumPackets; n++) {
        queue.write(new_packet(n * SamplesPerPacket, 0.11f));
    }

    expect_output(dp, NumPackets * SamplesPerPacket, 0.11f);
}

TEST(depacketizer, multiple_packets_multiple_reads) {
    enum { FramesPerPacket = 10 };

    CHECK(SamplesPerPacket % FramesPerPacket== 0);

    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(1 * SamplesPerPacket, 0.11f));
    queue.write(new_packet(2 * SamplesPerPacket, 0.22f));
    queue.write(new_packet(3 * SamplesPerPacket, 0.33f));

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, SamplesPerPacket / FramesPerPacket, 0.11f);
    }

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, SamplesPerPacket / FramesPerPacket, 0.22f);
    }

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, SamplesPerPacket / FramesPerPacket, 0.33f);
    }
}

TEST(depacketizer, timestamp_overflow) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    const packet::timestamp_t ts2 = 0;
    const packet::timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::timestamp_t ts3 = ts2 + SamplesPerPacket;

    queue.write(new_packet(ts1, 0.11f));
    queue.write(new_packet(ts2, 0.22f));
    queue.write(new_packet(ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.22f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, drop_late_packets) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    const packet::timestamp_t ts1 = SamplesPerPacket * 2;
    const packet::timestamp_t ts2 = SamplesPerPacket * 1;
    const packet::timestamp_t ts3 = SamplesPerPacket * 3;

    queue.write(new_packet(ts1, 0.11f));
    queue.write(new_packet(ts2, 0.22f));
    queue.write(new_packet(ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, drop_late_packets_timestamp_overflow) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    const packet::timestamp_t ts1 = 0;
    const packet::timestamp_t ts2 = ts1 - SamplesPerPacket;
    const packet::timestamp_t ts3 = ts1 + SamplesPerPacket;

    queue.write(new_packet(ts1, 0.11f));
    queue.write(new_packet(ts2, 0.22f));
    queue.write(new_packet(ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, zeros_no_packets) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    expect_output(dp, SamplesPerPacket, 0.00f);
}

TEST(depacketizer, zeros_no_next_packet) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(0, 0.11f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.00f);
}

TEST(depacketizer, zeros_between_packets) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(1 * SamplesPerPacket, 0.11f));
    queue.write(new_packet(3 * SamplesPerPacket, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.00f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, zeros_between_packets_timestamp_overflow) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    const packet::timestamp_t ts2 = 0;
    const packet::timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::timestamp_t ts3 = ts2 + SamplesPerPacket;

    queue.write(new_packet(ts1, 0.11f));
    queue.write(new_packet(ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.000f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, zeros_after_packet) {
    CHECK(SamplesPerPacket % 2 == 0);

    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(0, 0.11f));

    Frame f1 = new_frame(SamplesPerPacket / 2);
    Frame f2 = new_frame(SamplesPerPacket);

    dp.read(f1);
    dp.read(f2);

    expect_values(f1.samples().data(), SamplesPerPacket / 2 * NumCh, 0.11f);
    expect_values(f2.samples().data(), SamplesPerPacket / 2 * NumCh, 0.11f);
    expect_values(f2.samples().data() + SamplesPerPacket / 2 * NumCh,
                  SamplesPerPacket / 2 * NumCh, 0.00f);
}

TEST(depacketizer, packet_after_zeros) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    expect_output(dp, SamplesPerPacket, 0.00f);

    queue.write(new_packet(0, 0.11f));

    expect_output(dp, SamplesPerPacket, 0.11f);
}

TEST(depacketizer, overlapping_packets) {
    CHECK(SamplesPerPacket % 2 == 0);

    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    packet::timestamp_t ts1 = 0;
    packet::timestamp_t ts2 = SamplesPerPacket / 2;
    packet::timestamp_t ts3 = SamplesPerPacket;

    queue.write(new_packet(ts1, 0.11f));
    queue.write(new_packet(ts2, 0.22f));
    queue.write(new_packet(ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket / 2, 0.22f);
    expect_output(dp, SamplesPerPacket / 2, 0.33f);
}

TEST(depacketizer, frame_flags_packet_drops) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    packet::PacketPtr packets[] = {
        new_packet(SamplesPerPacket * 3, 0.11f),
        new_packet(SamplesPerPacket * 1, 0.11f),
        new_packet(SamplesPerPacket * 2, 0.11f),
        new_packet(SamplesPerPacket * 6, 0.11f),
        new_packet(SamplesPerPacket * 2, 0.11f),
        new_packet(SamplesPerPacket * 3, 0.11f),
    };

    unsigned frame_flags[] = {
        Frame::FlagFull,
        Frame::FlagPacketDrops | Frame::FlagEmpty,
        Frame::FlagEmpty,
        Frame::FlagFull,
        Frame::FlagPacketDrops | Frame::FlagEmpty,
        Frame::FlagEmpty,
    };

    CHECK(ROC_ARRAY_SIZE(packets) == ROC_ARRAY_SIZE(frame_flags));

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        queue.write(packets[n]);
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        Frame frame = new_frame(SamplesPerPacket);
        dp.read(frame);

        CHECK(frame.flags() == frame_flags[n]);
    }
}

TEST(depacketizer, frame_flags_packet_drops_full) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(2 * SamplesPerPacket, 0.11f));
    queue.write(new_packet(1 * SamplesPerPacket, 0.33f));
    queue.write(new_packet(3 * SamplesPerPacket, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    Frame frame = new_frame(SamplesPerPacket);
    dp.read(frame);

    CHECK(frame.flags() & Frame::FlagPacketDrops);
    CHECK(frame.flags() & Frame::FlagFull);
}

TEST(depacketizer, frame_flags_empty_full) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    packet::PacketPtr packets[] = {
        new_packet(SamplesPerPacket, 0.11f),
        new_packet(SamplesPerPacket * 2, 0.11f),
        NULL,
        new_packet(SamplesPerPacket * 5, 0.11f),
        new_packet(SamplesPerPacket * 7, 0.11f),
    };

    size_t frame_sizes[] = {
        SamplesPerPacket,
        SamplesPerPacket * 2,
        SamplesPerPacket,
        SamplesPerPacket * 2,
        SamplesPerPacket,
    };

    unsigned frame_flags[] = {
        Frame::FlagFull,
        0,
        Frame::FlagEmpty,
        0,
        Frame::FlagFull,
    };

    CHECK(ROC_ARRAY_SIZE(packets) == ROC_ARRAY_SIZE(frame_sizes));
    CHECK(ROC_ARRAY_SIZE(frame_sizes) == ROC_ARRAY_SIZE(frame_flags));

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        if (packets[n] != NULL) {
            queue.write(packets[n]);
        }

        Frame frame = new_frame(frame_sizes[n]);
        dp.read(frame);

        CHECK(frame.flags() == frame_flags[n]);
    }
}

TEST(depacketizer, frame_flags_packet_between_zeroes) {
    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    queue.write(new_packet(1 * SamplesPerPacket, 0.11f));
    queue.write(new_packet(3 * SamplesPerPacket, 0.22f));
    queue.write(new_packet(5 * SamplesPerPacket, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);

    Frame frame = new_frame(SamplesPerPacket * 3);
    dp.read(frame);

    CHECK(!(frame.flags() & Frame::FlagFull));
    CHECK(!(frame.flags() & Frame::FlagEmpty));

    UNSIGNED_LONGS_EQUAL(3 * SamplesPerPacket * NumCh, frame.samples().size());

    sample_t* buff_ptr = frame.samples().data();

    expect_values(buff_ptr, SamplesPerPacket * NumCh, 0.00f);
    expect_values(buff_ptr + SamplesPerPacket * NumCh, SamplesPerPacket * NumCh, 0.22f);
    expect_values(buff_ptr + 2 * SamplesPerPacket * NumCh, SamplesPerPacket * NumCh,
                  0.00f);

    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, timestamp) {
    enum {
        StartTimestamp = 1000,
        NumPackets = 3,
        FramesPerPacket = 10,
        SamplesPerFrame = SamplesPerPacket / FramesPerPacket
    };

    CHECK(SamplesPerPacket % FramesPerPacket== 0);

    packet::ConcurrentQueue queue(0, false);
    Depacketizer dp(queue, pcm_decoder, ChMask, false);

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f);

        CHECK(!dp.is_started());
        UNSIGNED_LONGS_EQUAL(0, dp.timestamp());
    }

    for (size_t n = 0; n < NumPackets; n++) {
        queue.write(
            new_packet(StartTimestamp + packet::timestamp_t(n * SamplesPerPacket), 0.1f));
    }

    packet::timestamp_t ts = StartTimestamp;

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.1f);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.timestamp());
    }

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.timestamp());
    }
}

} // namespace audio
} // namespace roc
