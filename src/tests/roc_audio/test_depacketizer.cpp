/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/depacketizer.h"
#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_audio/pcm_funcs.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/queue.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace audio {

namespace {

enum { MaxBufSize = 4000, SamplesPerPacket = 200, NumCh = 2, ChMask = 0x3 };

core::HeapAllocator allocator;
core::BufferPool<sample_t> sample_buffer_pool(allocator, MaxBufSize, true);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

rtp::Composer rtp_composer(NULL);

const audio::PcmFuncs& pcm_funcs = audio::PCM_int16_2ch;

} // namespace

TEST_GROUP(depacketizer) {
    packet::PacketPtr new_packet(
        IFrameEncoder& encoder, packet::timestamp_t ts, sample_t value) {
        packet::PacketPtr pp = new(packet_pool) packet::Packet(packet_pool);
        CHECK(pp);

        core::Slice<uint8_t> bp =
            new (byte_buffer_pool) core::Buffer<uint8_t>(byte_buffer_pool);
        CHECK(bp);

        CHECK(rtp_composer.prepare(*pp, bp, encoder.encoded_size(SamplesPerPacket)));

        pp->set_data(bp);

        pp->rtp()->timestamp = ts;
        pp->rtp()->duration = SamplesPerPacket;

        sample_t samples[SamplesPerPacket * NumCh];
        for (size_t n = 0; n < SamplesPerPacket * NumCh; n++) {
            samples[n] = value;
        }

        encoder.begin(pp->rtp()->payload.data(), pp->rtp()->payload.size());

        UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                             encoder.write(samples, SamplesPerPacket, ChMask));

        encoder.end();

        CHECK(rtp_composer.compose(*pp));

        return pp;
    }

    core::Slice<sample_t> new_buffer(size_t n_samples) {
        core::Slice<sample_t> buffer =
            new (sample_buffer_pool) core::Buffer<sample_t>(sample_buffer_pool);
        CHECK(buffer);
        buffer.resize(n_samples * NumCh);
        return buffer;
    }

    void expect_values(const sample_t* samples, size_t num_samples, sample_t value) {
        for (size_t n = 0; n < num_samples; n++) {
            DOUBLES_EQUAL((double)value, (double)samples[n], 0.0001);
        }
    }

    void expect_output(Depacketizer& depacketizer, size_t sz, sample_t value) {
        core::Slice<sample_t> buf = new_buffer(sz);

        Frame frame(buf.data(), buf.size());
        depacketizer.read(frame);

        UNSIGNED_LONGS_EQUAL(sz * NumCh, frame.size());
        expect_values(frame.data(), sz * NumCh, value);
    }

    void expect_flags(Depacketizer& depacketizer, size_t sz, unsigned int flags) {
        core::Slice<sample_t> buf = new_buffer(sz);

        Frame frame(buf.data(), buf.size());
        depacketizer.read(frame);

        UNSIGNED_LONGS_EQUAL(flags, frame.flags());
    }
};

TEST(depacketizer, one_packet_one_read) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    queue.write(new_packet(encoder, 0, 0.11f));

    expect_output(dp, SamplesPerPacket, 0.11f);
}

TEST(depacketizer, one_packet_multiple_reads) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    queue.write(new_packet(encoder, 0, 0.11f));

    for (size_t n = 0; n < SamplesPerPacket; n++) {
        expect_output(dp, 1, 0.11f);
    }
}

TEST(depacketizer, multiple_packets_one_read) {
    enum { NumPackets = 10 };

    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    for (packet::timestamp_t n = 0; n < NumPackets; n++) {
        queue.write(new_packet(encoder, n * SamplesPerPacket, 0.11f));
    }

    expect_output(dp, NumPackets * SamplesPerPacket, 0.11f);
}

TEST(depacketizer, multiple_packets_multiple_reads) {
    enum { FramesPerPacket = 10 };

    CHECK(SamplesPerPacket % FramesPerPacket== 0);

    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    queue.write(new_packet(encoder, 1 * SamplesPerPacket, 0.11f));
    queue.write(new_packet(encoder, 2 * SamplesPerPacket, 0.22f));
    queue.write(new_packet(encoder, 3 * SamplesPerPacket, 0.33f));

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
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    const packet::timestamp_t ts2 = 0;
    const packet::timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::timestamp_t ts3 = ts2 + SamplesPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f));
    queue.write(new_packet(encoder, ts2, 0.22f));
    queue.write(new_packet(encoder, ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.22f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, drop_late_packets) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    const packet::timestamp_t ts1 = SamplesPerPacket * 2;
    const packet::timestamp_t ts2 = SamplesPerPacket * 1;
    const packet::timestamp_t ts3 = SamplesPerPacket * 3;

    queue.write(new_packet(encoder, ts1, 0.11f));
    queue.write(new_packet(encoder, ts2, 0.22f));
    queue.write(new_packet(encoder, ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, drop_late_packets_timestamp_overflow) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    const packet::timestamp_t ts1 = 0;
    const packet::timestamp_t ts2 = ts1 - SamplesPerPacket;
    const packet::timestamp_t ts3 = ts1 + SamplesPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f));
    queue.write(new_packet(encoder, ts2, 0.22f));
    queue.write(new_packet(encoder, ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, zeros_no_packets) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    expect_output(dp, SamplesPerPacket, 0.00f);
}

TEST(depacketizer, zeros_no_next_packet) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    queue.write(new_packet(encoder, 0, 0.11f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.00f);
}

TEST(depacketizer, zeros_between_packets) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    queue.write(new_packet(encoder, 1 * SamplesPerPacket, 0.11f));
    queue.write(new_packet(encoder, 3 * SamplesPerPacket, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.00f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, zeros_between_packets_timestamp_overflow) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    const packet::timestamp_t ts2 = 0;
    const packet::timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::timestamp_t ts3 = ts2 + SamplesPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f));
    queue.write(new_packet(encoder, ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket, 0.000f);
    expect_output(dp, SamplesPerPacket, 0.33f);
}

TEST(depacketizer, zeros_after_packet) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    CHECK(SamplesPerPacket % 2 == 0);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    queue.write(new_packet(encoder, 0, 0.11f));

    core::Slice<sample_t> b1 = new_buffer(SamplesPerPacket / 2);
    core::Slice<sample_t> b2 = new_buffer(SamplesPerPacket);

    Frame f1(b1.data(), b1.size());
    Frame f2(b2.data(), b2.size());

    dp.read(f1);
    dp.read(f2);

    expect_values(f1.data(), SamplesPerPacket / 2 * NumCh, 0.11f);
    expect_values(f2.data(), SamplesPerPacket / 2 * NumCh, 0.11f);
    expect_values(f2.data() + SamplesPerPacket / 2 * NumCh, SamplesPerPacket / 2 * NumCh,
                  0.00f);
}

TEST(depacketizer, packet_after_zeros) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    expect_output(dp, SamplesPerPacket, 0.00f);

    queue.write(new_packet(encoder, 0, 0.11f));

    expect_output(dp, SamplesPerPacket, 0.11f);
}

TEST(depacketizer, overlapping_packets) {
    CHECK(SamplesPerPacket % 2 == 0);

    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    packet::timestamp_t ts1 = 0;
    packet::timestamp_t ts2 = SamplesPerPacket / 2;
    packet::timestamp_t ts3 = SamplesPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f));
    queue.write(new_packet(encoder, ts2, 0.22f));
    queue.write(new_packet(encoder, ts3, 0.33f));

    expect_output(dp, SamplesPerPacket, 0.11f);
    expect_output(dp, SamplesPerPacket / 2, 0.22f);
    expect_output(dp, SamplesPerPacket / 2, 0.33f);
}

TEST(depacketizer, frame_flags_incompltete_blank) {
    enum { PacketsPerFrame = 3 };

    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    packet::PacketPtr packets[][PacketsPerFrame] = {
        {
            new_packet(encoder, SamplesPerPacket * 1, 0.11f),
            new_packet(encoder, SamplesPerPacket * 2, 0.11f),
            new_packet(encoder, SamplesPerPacket * 3, 0.11f),
        },
        {
            NULL,
            new_packet(encoder, SamplesPerPacket * 5, 0.11f),
            new_packet(encoder, SamplesPerPacket * 6, 0.11f),
        },
        {
            new_packet(encoder, SamplesPerPacket * 7, 0.11f),
            NULL,
            new_packet(encoder, SamplesPerPacket * 9, 0.11f),
        },
        {
            new_packet(encoder, SamplesPerPacket * 10, 0.11f),
            new_packet(encoder, SamplesPerPacket * 11, 0.11f),
            NULL,
        },
        {
            NULL,
            new_packet(encoder, SamplesPerPacket * 14, 0.11f),
            NULL,
        },
        {
            NULL, NULL, NULL,
        },
        {
            new_packet(encoder, SamplesPerPacket * 22, 0.11f),
            new_packet(encoder, SamplesPerPacket * 23, 0.11f),
            new_packet(encoder, SamplesPerPacket * 24, 0.11f),
        },
        {
            NULL, NULL, NULL,
        },
    };

    unsigned frame_flags[] = {
        0,
        Frame::FlagIncomplete,
        Frame::FlagIncomplete,
        Frame::FlagIncomplete,
        Frame::FlagIncomplete,
        Frame::FlagIncomplete | Frame::FlagBlank,
        Frame::FlagIncomplete | Frame::FlagBlank,
        0,
    };

    CHECK(ROC_ARRAY_SIZE(packets) == ROC_ARRAY_SIZE(frame_flags));

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        for (size_t p = 0; p < PacketsPerFrame; p++) {
            if (packets[n][p] != NULL) {
                queue.write(packets[n][p]);
            }
        }

        expect_flags(dp, SamplesPerPacket * PacketsPerFrame, frame_flags[n]);
    }
}

TEST(depacketizer, frame_flags_drops) {
    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    packet::PacketPtr packets[] = {
        new_packet(encoder, SamplesPerPacket * 4, 0.11f),
        new_packet(encoder, SamplesPerPacket * 1, 0.11f),
        new_packet(encoder, SamplesPerPacket * 2, 0.11f),
        new_packet(encoder, SamplesPerPacket * 5, 0.11f),
        new_packet(encoder, SamplesPerPacket * 6, 0.11f),
        new_packet(encoder, SamplesPerPacket * 3, 0.11f),
        new_packet(encoder, SamplesPerPacket * 8, 0.11f),
    };

    unsigned frame_flags[] = {
        0,                //
        Frame::FlagDrops, //
        0,                //
        Frame::FlagIncomplete | Frame::FlagBlank | Frame::FlagDrops,
        0,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        queue.write(packets[n]);
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(frame_flags); n++) {
        expect_flags(dp, SamplesPerPacket, frame_flags[n]);
    }
}

TEST(depacketizer, timestamp) {
    enum {
        StartTimestamp = 1000,
        NumPackets = 3,
        FramesPerPacket = 10,
        SamplesPerFrame = SamplesPerPacket / FramesPerPacket
    };

    CHECK(SamplesPerPacket % FramesPerPacket== 0);

    audio::PcmEncoder encoder(pcm_funcs);
    audio::PcmDecoder decoder(pcm_funcs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, ChMask, false);

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f);

        CHECK(!dp.started());
        UNSIGNED_LONGS_EQUAL(0, dp.timestamp());
    }

    for (size_t n = 0; n < NumPackets; n++) {
        queue.write(new_packet(encoder,
                               StartTimestamp + packet::timestamp_t(n * SamplesPerPacket),
                               0.1f));
    }

    packet::timestamp_t ts = StartTimestamp;

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.1f);

        ts += SamplesPerFrame;

        CHECK(dp.started());
        UNSIGNED_LONGS_EQUAL(ts, dp.timestamp());
    }

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f);

        ts += SamplesPerFrame;

        CHECK(dp.started());
        UNSIGNED_LONGS_EQUAL(ts, dp.timestamp());
    }
}

} // namespace audio
} // namespace roc
