/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_rtp/composer.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxBufSize = 4000,
    SamplesPerPacket = 200,
    SampleRate = 100,
    ChMask = 0x3,
    NumCh = 2,
    SamplesSize = SamplesPerPacket * NumCh
};

const SampleSpec SampleSpecs(SampleRate, ChanLayout_Surround, ChanOrder_Smpte, ChMask);
const PcmFormat PcmFmt(PcmEncoding_SInt16, PcmEndian_Big);
const core::nanoseconds_t NsPerPacket = SampleSpecs.samples_overall_2_ns(SamplesSize);
const core::nanoseconds_t Now = 1691499037871419405;

core::HeapArena arena;
core::BufferFactory<sample_t> sample_buffer_factory(arena, MaxBufSize);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, MaxBufSize);
packet::PacketFactory packet_factory(arena);

rtp::Composer rtp_composer(NULL);

packet::PacketPtr new_packet(IFrameEncoder& encoder,
                             packet::stream_timestamp_t ts,
                             sample_t value,
                             core::nanoseconds_t capt_ts) {
    packet::PacketPtr pp = packet_factory.new_packet();
    CHECK(pp);

    core::Slice<uint8_t> bp = byte_buffer_factory.new_buffer();
    CHECK(bp);

    CHECK(rtp_composer.prepare(*pp, bp, encoder.encoded_byte_count(SamplesPerPacket)));

    pp->set_data(bp);

    pp->rtp()->stream_timestamp = ts;
    pp->rtp()->duration = SamplesPerPacket;
    pp->rtp()->capture_timestamp = capt_ts;

    sample_t samples[SamplesSize];
    for (size_t n = 0; n < SamplesSize; n++) {
        samples[n] = value;
    }

    encoder.begin(pp->rtp()->payload.data(), pp->rtp()->payload.size());

    UNSIGNED_LONGS_EQUAL(SamplesPerPacket, encoder.write(samples, SamplesPerPacket));

    encoder.end();

    CHECK(rtp_composer.compose(*pp));

    return pp;
}

core::Slice<sample_t> new_buffer(size_t n_samples) {
    core::Slice<sample_t> buffer = sample_buffer_factory.new_buffer();
    CHECK(buffer);
    buffer.reslice(0, n_samples * SampleSpecs.num_channels());
    return buffer;
}

void expect_values(const sample_t* samples, size_t num_samples, sample_t value) {
    for (size_t n = 0; n < num_samples; n++) {
        DOUBLES_EQUAL((double)value, (double)samples[n], 0.0001);
    }
}

void expect_output(Depacketizer& depacketizer,
                   size_t sz,
                   sample_t value,
                   core::nanoseconds_t capt_ts) {
    core::Slice<sample_t> buf = new_buffer(sz);

    Frame frame(buf.data(), buf.size());
    CHECK(depacketizer.read(frame));

    CHECK(core::ns_equal_delta(frame.capture_timestamp(), capt_ts, core::Microsecond));
    UNSIGNED_LONGS_EQUAL(sz * SampleSpecs.num_channels(), frame.num_samples());
    expect_values(frame.samples(), sz * SampleSpecs.num_channels(), value);
}

void expect_flags(Depacketizer& depacketizer,
                  size_t sz,
                  unsigned int flags,
                  core::nanoseconds_t capt_ts = -1) {
    core::Slice<sample_t> buf = new_buffer(sz);
    const core::nanoseconds_t epsilon = 100 * core::Microsecond;

    Frame frame(buf.data(), buf.size());
    CHECK(depacketizer.read(frame));

    UNSIGNED_LONGS_EQUAL(flags, frame.flags());
    if (capt_ts >= 0) {
        CHECK(core::ns_equal_delta(frame.capture_timestamp(), capt_ts, epsilon));
    }
}

class TestReader : public packet::IReader {
public:
    explicit TestReader(packet::IReader& reader)
        : reader_(reader)
        , call_count_(0)
        , code_enabled_(false)
        , code_(default_code_) {
    }

    virtual status::StatusCode read(packet::PacketPtr& pp) {
        ++call_count_;

        if (code_enabled_) {
            return code_;
        }

        return reader_.read(pp);
    }

    void enable_status_code(status::StatusCode code) {
        code_enabled_ = true;
        code_ = code;
    }

    void disable_status_code() {
        code_enabled_ = false;
        code_ = default_code_;
    }

    unsigned call_count() const {
        return call_count_;
    }

private:
    static const status::StatusCode default_code_ = status::StatusUnknown;

    packet::IReader& reader_;

    unsigned call_count_;
    bool code_enabled_;
    status::StatusCode code_;
};

} // namespace

TEST_GROUP(depacketizer) {};

TEST(depacketizer, one_packet_one_read) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    queue.write(new_packet(encoder, 0, 0.11f, Now));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
}

TEST(depacketizer, one_packet_multiple_reads) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    queue.write(new_packet(encoder, 0, 0.11f, Now));

    core::nanoseconds_t ts = Now;
    for (size_t n = 0; n < SamplesPerPacket; n++) {
        expect_output(dp, 1, 0.11f, ts);
        ts += SampleSpecs.samples_per_chan_2_ns(1);
    }
}

TEST(depacketizer, multiple_packets_one_read) {
    enum { NumPackets = 10 };

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    core::nanoseconds_t ts = Now;
    for (packet::stream_timestamp_t n = 0; n < NumPackets; n++) {
        queue.write(new_packet(encoder, n * SamplesPerPacket, 0.11f, ts));
        ts += NsPerPacket;
    }

    expect_output(dp, NumPackets * SamplesPerPacket, 0.11f, Now);
}

TEST(depacketizer, multiple_packets_multiple_reads) {
    enum { FramesPerPacket = 10 };

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    // Start with a packet with zero capture timestamp.
    queue.write(new_packet(encoder, 0, 0.01f, 0));
    const size_t samples_per_frame = SamplesPerPacket / FramesPerPacket;
    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.01f, 0);
    }

    core::nanoseconds_t ts = Now;
    queue.write(new_packet(encoder, 1 * SamplesPerPacket, 0.11f, ts));
    ts += NsPerPacket;
    queue.write(new_packet(encoder, 2 * SamplesPerPacket, 0.22f, ts));
    ts += NsPerPacket;
    queue.write(new_packet(encoder, 3 * SamplesPerPacket, 0.33f, ts));

    ts = Now;
    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.11f, ts);
        ts += SampleSpecs.samples_per_chan_2_ns(samples_per_frame);
    }

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.22f, ts);
        ts += SampleSpecs.samples_per_chan_2_ns(samples_per_frame);
    }

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.33f, ts);
        ts += SampleSpecs.samples_per_chan_2_ns(samples_per_frame);
    }
}

TEST(depacketizer, timestamp_overflow) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    const packet::stream_timestamp_t ts2 = 0;
    const packet::stream_timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts2 + SamplesPerPacket;

    core::nanoseconds_t ts = Now;
    queue.write(new_packet(encoder, ts1, 0.11f, ts));
    ts += NsPerPacket;
    queue.write(new_packet(encoder, ts2, 0.22f, ts));
    ts += NsPerPacket;
    queue.write(new_packet(encoder, ts3, 0.33f, ts));

    ts = Now;
    expect_output(dp, SamplesPerPacket, 0.11f, ts);
    ts += NsPerPacket;
    expect_output(dp, SamplesPerPacket, 0.22f, ts);
    ts += NsPerPacket;
    expect_output(dp, SamplesPerPacket, 0.33f, ts);
}

TEST(depacketizer, drop_late_packets) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    const packet::stream_timestamp_t ts1 = SamplesPerPacket * 2;
    const packet::stream_timestamp_t ts2 = SamplesPerPacket * 1;
    const packet::stream_timestamp_t ts3 = SamplesPerPacket * 3;
    const core::nanoseconds_t capt_ts1 = Now + NsPerPacket;
    const core::nanoseconds_t capt_ts2 = Now;
    const core::nanoseconds_t capt_ts3 = ts1 + NsPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f, capt_ts1));
    queue.write(new_packet(encoder, ts2, 0.22f, capt_ts2));
    queue.write(new_packet(encoder, ts3, 0.33f, capt_ts3));

    expect_output(dp, SamplesPerPacket, 0.11f, capt_ts1);
    expect_output(dp, SamplesPerPacket, 0.33f, capt_ts3);
}

TEST(depacketizer, drop_late_packets_timestamp_overflow) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    const packet::stream_timestamp_t ts1 = 0;
    const packet::stream_timestamp_t ts2 = ts1 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts1 + SamplesPerPacket;
    const core::nanoseconds_t capt_ts1 = Now;
    const core::nanoseconds_t capt_ts2 = Now - NsPerPacket;
    const core::nanoseconds_t capt_ts3 = Now + NsPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f, capt_ts1));
    queue.write(new_packet(encoder, ts2, 0.22f, capt_ts2));
    queue.write(new_packet(encoder, ts3, 0.33f, capt_ts3));

    expect_output(dp, SamplesPerPacket, 0.11f, capt_ts1);
    expect_output(dp, SamplesPerPacket, 0.33f, capt_ts3);
}

TEST(depacketizer, zeros_no_packets) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    expect_output(dp, SamplesPerPacket, 0.00f, 0);
}

TEST(depacketizer, zeros_no_next_packet) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    queue.write(new_packet(encoder, 0, 0.11f, 0));

    expect_output(dp, SamplesPerPacket, 0.11f, 0);
    expect_output(dp, SamplesPerPacket, 0.00f, 0); // no packet -- no ts
}

TEST(depacketizer, zeros_between_packets) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    queue.write(new_packet(encoder, 1 * SamplesPerPacket, 0.11f, Now));
    queue.write(new_packet(encoder, 3 * SamplesPerPacket, 0.33f, Now + NsPerPacket * 2));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
    expect_output(dp, SamplesPerPacket, 0.00f, Now + NsPerPacket);
    expect_output(dp, SamplesPerPacket, 0.33f, Now + 2 * NsPerPacket);
}

TEST(depacketizer, zeros_between_packets_timestamp_overflow) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    const packet::stream_timestamp_t ts2 = 0;
    const packet::stream_timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts2 + SamplesPerPacket;
    const core::nanoseconds_t capt_ts1 = Now - NsPerPacket;
    const core::nanoseconds_t capt_ts2 = Now;
    const core::nanoseconds_t capt_ts3 = Now + NsPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f, capt_ts1));
    queue.write(new_packet(encoder, ts3, 0.33f, capt_ts3));

    expect_output(dp, SamplesPerPacket, 0.11f, capt_ts1);
    expect_output(dp, SamplesPerPacket, 0.000f, capt_ts2);
    expect_output(dp, SamplesPerPacket, 0.33f, capt_ts3);
}

TEST(depacketizer, zeros_after_packet) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    CHECK(SamplesPerPacket % 2 == 0);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    queue.write(new_packet(encoder, 0, 0.11f, Now));

    core::Slice<sample_t> b1 = new_buffer(SamplesPerPacket / 2);
    core::Slice<sample_t> b2 = new_buffer(SamplesPerPacket);

    Frame f1(b1.data(), b1.size());
    Frame f2(b2.data(), b2.size());

    dp.read(f1);
    dp.read(f2);

    expect_values(f1.samples(), SamplesPerPacket / 2 * SampleSpecs.num_channels(), 0.11f);
    expect_values(f2.samples(), SamplesPerPacket / 2 * SampleSpecs.num_channels(), 0.11f);
    expect_values(f2.samples() + SamplesPerPacket / 2 * SampleSpecs.num_channels(),
                  SamplesPerPacket / 2 * SampleSpecs.num_channels(), 0.00f);
}

TEST(depacketizer, packet_after_zeros) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    expect_output(dp, SamplesPerPacket, 0.00f, 0);

    queue.write(new_packet(encoder, 0, 0.11f, Now));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
}

TEST(depacketizer, overlapping_packets) {
    CHECK(SamplesPerPacket % 2 == 0);

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    packet::stream_timestamp_t ts1 = 0;
    packet::stream_timestamp_t ts2 = SamplesPerPacket / 2;
    packet::stream_timestamp_t ts3 = SamplesPerPacket;

    queue.write(new_packet(encoder, ts1, 0.11f, Now));
    queue.write(new_packet(encoder, ts2, 0.22f, Now + NsPerPacket / 2));
    queue.write(new_packet(encoder, ts3, 0.33f, Now + NsPerPacket));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
    expect_output(dp, SamplesPerPacket / 2, 0.22f, Now + NsPerPacket);
    expect_output(dp, SamplesPerPacket / 2, 0.33f, Now + NsPerPacket * 3 / 2);
}

TEST(depacketizer, frame_flags_incompltete_blank) {
    enum { PacketsPerFrame = 3 };

    PcmEncoder encoder(PcmFmt, SampleSpecs);

    packet::Queue queue;

    packet::PacketPtr packets[][PacketsPerFrame] = {
        {
            new_packet(encoder, SamplesPerPacket * 1, 0.11f, Now),
            new_packet(encoder, SamplesPerPacket * 2, 0.11f, Now + NsPerPacket),
            new_packet(encoder, SamplesPerPacket * 3, 0.11f, Now + 2 * NsPerPacket),
        },
        {
            NULL,
            new_packet(encoder, SamplesPerPacket * 5, 0.11f, Now + NsPerPacket),
            new_packet(encoder, SamplesPerPacket * 6, 0.11f, Now + 2 * NsPerPacket),
        },
        {
            new_packet(encoder, SamplesPerPacket * 7, 0.11f, Now),
            NULL,
            new_packet(encoder, SamplesPerPacket * 9, 0.11f, Now + 2 * NsPerPacket),
        },
        {
            new_packet(encoder, SamplesPerPacket * 10, 0.11f, Now),
            new_packet(encoder, SamplesPerPacket * 11, 0.11f, Now + NsPerPacket),
            NULL,
        },
        {
            NULL,
            new_packet(encoder, SamplesPerPacket * 14, 0.11f, Now + NsPerPacket),
            NULL,
        },
        {
            NULL,
            NULL,
            NULL,
        },
        {
            new_packet(encoder, SamplesPerPacket * 22, 0.11f, Now),
            new_packet(encoder, SamplesPerPacket * 23, 0.11f, Now + NsPerPacket),
            new_packet(encoder, SamplesPerPacket * 24, 0.11f, Now + 2 * NsPerPacket),
        },
        {
            NULL,
            NULL,
            NULL,
        },
    };

    unsigned frame_flags[] = {
        Frame::FlagNonblank,
        Frame::FlagIncomplete | Frame::FlagNonblank,
        Frame::FlagIncomplete | Frame::FlagNonblank,
        Frame::FlagIncomplete | Frame::FlagNonblank,
        Frame::FlagIncomplete | Frame::FlagNonblank,
        Frame::FlagIncomplete,
        Frame::FlagNonblank,
        Frame::FlagIncomplete,
    };

    core::nanoseconds_t capt_ts[] = {
        Now, Now + NsPerPacket, Now, Now, Now + NsPerPacket, 0, Now, 0,
    };

    CHECK(ROC_ARRAY_SIZE(packets) == ROC_ARRAY_SIZE(frame_flags));

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        PcmDecoder decoder(PcmFmt, SampleSpecs);
        Depacketizer dp(queue, decoder, SampleSpecs, false);
        CHECK(dp.is_valid());

        for (size_t p = 0; p < PacketsPerFrame; p++) {
            if (packets[n][p] != NULL) {
                queue.write(packets[n][p]);
            }
        }

        expect_flags(dp, SamplesPerPacket * PacketsPerFrame, frame_flags[n], capt_ts[n]);
    }
}

TEST(depacketizer, frame_flags_drops) {
    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    packet::PacketPtr packets[] = {
        new_packet(encoder, SamplesPerPacket * 4, 0.11f, 0),
        new_packet(encoder, SamplesPerPacket * 1, 0.11f, 0),
        new_packet(encoder, SamplesPerPacket * 2, 0.11f, 0),
        new_packet(encoder, SamplesPerPacket * 5, 0.11f, 0),
        new_packet(encoder, SamplesPerPacket * 6, 0.11f, 0),
        new_packet(encoder, SamplesPerPacket * 3, 0.11f, 0),
        new_packet(encoder, SamplesPerPacket * 8, 0.11f, 0),
    };

    unsigned frame_flags[] = {
        Frame::FlagNonblank,                      //
        Frame::FlagNonblank | Frame::FlagDrops,   //
        Frame::FlagNonblank,                      //
        Frame::FlagIncomplete | Frame::FlagDrops, //
        Frame::FlagNonblank,                      //
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

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, SampleSpecs, false);
    CHECK(dp.is_valid());

    core::nanoseconds_t capt_ts = 0;
    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f, 0);
        capt_ts += SampleSpecs.samples_per_chan_2_ns(SamplesPerFrame);

        CHECK(!dp.is_started());
        UNSIGNED_LONGS_EQUAL(0, dp.next_timestamp());
    }

    capt_ts = Now;
    for (size_t n = 0; n < NumPackets; n++) {
        const size_t nsamples = packet::stream_timestamp_t(n * SamplesPerPacket);
        queue.write(new_packet(encoder, StartTimestamp + nsamples, 0.1f, capt_ts));
        capt_ts += SampleSpecs.samples_per_chan_2_ns(SamplesPerPacket);
    }

    packet::stream_timestamp_t ts = StartTimestamp;

    capt_ts = Now;
    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.1f, capt_ts);
        capt_ts += SampleSpecs.samples_per_chan_2_ns(SamplesPerFrame);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.next_timestamp());
    }

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f, capt_ts);
        capt_ts += SampleSpecs.samples_per_chan_2_ns(SamplesPerFrame);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.next_timestamp());
    }
}

TEST(depacketizer, read_after_error) {
    const status::StatusCode codes[] = {
        status::StatusUnknown,
        status::StatusNoData,
    };

    for (unsigned n = 0; n < ROC_ARRAY_SIZE(codes); ++n) {
        PcmEncoder encoder(PcmFmt, SampleSpecs);
        PcmDecoder decoder(PcmFmt, SampleSpecs);

        packet::Queue queue;
        TestReader reader(queue);
        Depacketizer dp(reader, decoder, SampleSpecs, false);
        CHECK(dp.is_valid());

        queue.write(new_packet(encoder, 0, 0.11f, Now));

        UNSIGNED_LONGS_EQUAL(0, reader.call_count());

        reader.enable_status_code(codes[n]);
        expect_output(dp, SamplesPerPacket, 0.00f, 0);
        UNSIGNED_LONGS_EQUAL(1, reader.call_count());
        CHECK(dp.is_valid());

        reader.disable_status_code();
        expect_output(dp, SamplesPerPacket, 0.11f, Now);
        UNSIGNED_LONGS_EQUAL(2, reader.call_count());
        CHECK(dp.is_valid());
    }
}

} // namespace audio
} // namespace roc
