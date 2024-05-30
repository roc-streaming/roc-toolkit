/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/depacketizer.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
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
    SamplesPerPacket = 200, // per channel
    SampleRate = 100,

    NumCh = 2,
    ChMask = 0x3,

    MaxBufSize = 16000,
};

const SampleSpec frame_spec(
    SampleRate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

const SampleSpec packet_spec(
    SampleRate, PcmFormat_SInt16_Be, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

const core::nanoseconds_t NsPerPacket =
    packet_spec.samples_per_chan_2_ns(SamplesPerPacket);
const core::nanoseconds_t Now = 1691499037871419405;

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, MaxBufSize);
FrameFactory frame_factory(arena, MaxBufSize);

rtp::Composer rtp_composer(NULL);

packet::PacketPtr new_packet(IFrameEncoder& encoder,
                             packet::stream_timestamp_t ts,
                             sample_t value,
                             core::nanoseconds_t capt_ts) {
    packet::PacketPtr pp = packet_factory.new_packet();
    CHECK(pp);

    core::Slice<uint8_t> bp = packet_factory.new_packet_buffer();
    CHECK(bp);

    CHECK(rtp_composer.prepare(*pp, bp, encoder.encoded_byte_count(SamplesPerPacket)));

    pp->set_buffer(bp);

    pp->rtp()->stream_timestamp = ts;
    pp->rtp()->duration = SamplesPerPacket;
    pp->rtp()->capture_timestamp = capt_ts;

    sample_t samples[SamplesPerPacket * NumCh];
    for (size_t n = 0; n < SamplesPerPacket * NumCh; n++) {
        samples[n] = value;
    }

    encoder.begin(pp->rtp()->payload.data(), pp->rtp()->payload.size());

    UNSIGNED_LONGS_EQUAL(SamplesPerPacket, encoder.write(samples, SamplesPerPacket));

    encoder.end();

    CHECK(rtp_composer.compose(*pp));

    return pp;
}

void write_packet(packet::IWriter& writer, packet::PacketPtr packet) {
    CHECK(packet);
    LONGS_EQUAL(status::StatusOK, writer.write(packet));
}

void expect_values(const sample_t* samples, size_t num_samples, sample_t value) {
    for (size_t n = 0; n < num_samples; n++) {
        DOUBLES_EQUAL((double)value, (double)samples[n], 0.0001);
    }
}

void expect_output(Depacketizer& depacketizer,
                   size_t samples_per_chan,
                   sample_t value,
                   core::nanoseconds_t capt_ts) {
    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(status::StatusOK,
                depacketizer.read(*frame, (packet::stream_timestamp_t)samples_per_chan));

    CHECK(frame->is_raw());

    UNSIGNED_LONGS_EQUAL(samples_per_chan, frame->duration());
    UNSIGNED_LONGS_EQUAL(samples_per_chan * frame_spec.num_channels(),
                         frame->num_raw_samples());

    CHECK(core::ns_equal_delta(frame->capture_timestamp(), capt_ts, core::Microsecond));

    expect_values(frame->raw_samples(), samples_per_chan * frame_spec.num_channels(),
                  value);
}

void expect_partial(Depacketizer& depacketizer,
                    size_t requested_samples_per_chan,
                    size_t expected_samples_per_chan,
                    sample_t value,
                    core::nanoseconds_t capt_ts) {
    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(status::StatusPart,
                depacketizer.read(
                    *frame, (packet::stream_timestamp_t)requested_samples_per_chan));

    CHECK(frame->is_raw());

    UNSIGNED_LONGS_EQUAL(expected_samples_per_chan, frame->duration());
    UNSIGNED_LONGS_EQUAL(expected_samples_per_chan * frame_spec.num_channels(),
                         frame->num_raw_samples());

    CHECK(core::ns_equal_delta(frame->capture_timestamp(), capt_ts, core::Microsecond));

    expect_values(frame->raw_samples(),
                  expected_samples_per_chan * frame_spec.num_channels(), value);
}

void expect_flags(Depacketizer& depacketizer,
                  size_t samples_per_chan,
                  unsigned int flags,
                  core::nanoseconds_t capt_ts = -1) {
    const core::nanoseconds_t epsilon = 100 * core::Microsecond;

    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(status::StatusOK,
                depacketizer.read(*frame, (packet::stream_timestamp_t)samples_per_chan));

    UNSIGNED_LONGS_EQUAL(flags, frame->flags());
    if (capt_ts >= 0) {
        CHECK(core::ns_equal_delta(frame->capture_timestamp(), capt_ts, epsilon));
    }
}

void expect_error(Depacketizer& depacketizer,
                  size_t samples_per_chan,
                  status::StatusCode expected_status) {
    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(expected_status,
                depacketizer.read(*frame, (packet::stream_timestamp_t)samples_per_chan));
}

class StatusReader : public packet::IReader {
public:
    explicit StatusReader(packet::IReader& reader)
        : reader_(reader)
        , code_(status::NoStatus) {
    }

    virtual status::StatusCode read(packet::PacketPtr& pp) {
        if (code_ != status::NoStatus && code_ != status::StatusOK) {
            return code_;
        }
        return reader_.read(pp);
    }

    void set_status(status::StatusCode code) {
        code_ = code;
    }

private:
    packet::IReader& reader_;
    status::StatusCode code_;
};

} // namespace

TEST_GROUP(depacketizer) {};

TEST(depacketizer, one_packet_one_read) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
}

TEST(depacketizer, one_packet_multiple_reads) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));

    core::nanoseconds_t ts = Now;
    for (size_t n = 0; n < SamplesPerPacket; n++) {
        expect_output(dp, 1, 0.11f, ts);
        ts += frame_spec.samples_per_chan_2_ns(1);
    }
}

TEST(depacketizer, multiple_packets_one_read) {
    enum { NumPackets = 10 };

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t ts = Now;
    for (packet::stream_timestamp_t n = 0; n < NumPackets; n++) {
        write_packet(queue, new_packet(encoder, n * SamplesPerPacket, 0.11f, ts));
        ts += NsPerPacket;
    }

    expect_output(dp, NumPackets * SamplesPerPacket, 0.11f, Now);
}

TEST(depacketizer, multiple_packets_multiple_reads) {
    enum { FramesPerPacket = 10 };

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    // Start with a packet with zero capture timestamp.
    write_packet(queue, new_packet(encoder, 0, 0.01f, 0));
    const size_t samples_per_frame = SamplesPerPacket / FramesPerPacket;
    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.01f, 0);
    }

    core::nanoseconds_t ts = Now;
    write_packet(queue, new_packet(encoder, 1 * SamplesPerPacket, 0.11f, ts));
    ts += NsPerPacket;
    write_packet(queue, new_packet(encoder, 2 * SamplesPerPacket, 0.22f, ts));
    ts += NsPerPacket;
    write_packet(queue, new_packet(encoder, 3 * SamplesPerPacket, 0.33f, ts));

    ts = Now;
    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.11f, ts);
        ts += frame_spec.samples_per_chan_2_ns(samples_per_frame);
    }

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.22f, ts);
        ts += frame_spec.samples_per_chan_2_ns(samples_per_frame);
    }

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(dp, samples_per_frame, 0.33f, ts);
        ts += frame_spec.samples_per_chan_2_ns(samples_per_frame);
    }
}

TEST(depacketizer, timestamp_overflow) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const packet::stream_timestamp_t ts2 = 0;
    const packet::stream_timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts2 + SamplesPerPacket;

    core::nanoseconds_t ts = Now;
    write_packet(queue, new_packet(encoder, ts1, 0.11f, ts));
    ts += NsPerPacket;
    write_packet(queue, new_packet(encoder, ts2, 0.22f, ts));
    ts += NsPerPacket;
    write_packet(queue, new_packet(encoder, ts3, 0.33f, ts));

    ts = Now;
    expect_output(dp, SamplesPerPacket, 0.11f, ts);
    ts += NsPerPacket;
    expect_output(dp, SamplesPerPacket, 0.22f, ts);
    ts += NsPerPacket;
    expect_output(dp, SamplesPerPacket, 0.33f, ts);
}

TEST(depacketizer, drop_late_packets) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const packet::stream_timestamp_t ts1 = SamplesPerPacket * 2;
    const packet::stream_timestamp_t ts2 = SamplesPerPacket * 1;
    const packet::stream_timestamp_t ts3 = SamplesPerPacket * 3;
    const core::nanoseconds_t capt_ts1 = Now + NsPerPacket;
    const core::nanoseconds_t capt_ts2 = Now;
    const core::nanoseconds_t capt_ts3 = ts1 + NsPerPacket;

    write_packet(queue, new_packet(encoder, ts1, 0.11f, capt_ts1));
    write_packet(queue, new_packet(encoder, ts2, 0.22f, capt_ts2));
    write_packet(queue, new_packet(encoder, ts3, 0.33f, capt_ts3));

    expect_output(dp, SamplesPerPacket, 0.11f, capt_ts1);
    expect_output(dp, SamplesPerPacket, 0.33f, capt_ts3);
}

TEST(depacketizer, drop_late_packets_timestamp_overflow) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const packet::stream_timestamp_t ts1 = 0;
    const packet::stream_timestamp_t ts2 = ts1 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts1 + SamplesPerPacket;
    const core::nanoseconds_t capt_ts1 = Now;
    const core::nanoseconds_t capt_ts2 = Now - NsPerPacket;
    const core::nanoseconds_t capt_ts3 = Now + NsPerPacket;

    write_packet(queue, new_packet(encoder, ts1, 0.11f, capt_ts1));
    write_packet(queue, new_packet(encoder, ts2, 0.22f, capt_ts2));
    write_packet(queue, new_packet(encoder, ts3, 0.33f, capt_ts3));

    expect_output(dp, SamplesPerPacket, 0.11f, capt_ts1);
    expect_output(dp, SamplesPerPacket, 0.33f, capt_ts3);
}

TEST(depacketizer, zeros_no_packets) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    expect_output(dp, SamplesPerPacket, 0.00f, 0);
}

TEST(depacketizer, zeros_no_next_packet) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, 0));

    expect_output(dp, SamplesPerPacket, 0.11f, 0);
    expect_output(dp, SamplesPerPacket, 0.00f, 0); // no packet -- no ts
}

TEST(depacketizer, zeros_between_packets) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const core::nanoseconds_t capt_ts1 = Now;
    const core::nanoseconds_t capt_ts2 = Now + NsPerPacket * 2;

    write_packet(queue, new_packet(encoder, 1 * SamplesPerPacket, 0.11f, capt_ts1));
    write_packet(queue, new_packet(encoder, 3 * SamplesPerPacket, 0.33f, capt_ts2));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
    expect_output(dp, SamplesPerPacket, 0.00f, Now + NsPerPacket);
    expect_output(dp, SamplesPerPacket, 0.33f, Now + 2 * NsPerPacket);
}

TEST(depacketizer, zeros_between_packets_timestamp_overflow) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const packet::stream_timestamp_t ts2 = 0;
    const packet::stream_timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts2 + SamplesPerPacket;
    const core::nanoseconds_t capt_ts1 = Now - NsPerPacket;
    const core::nanoseconds_t capt_ts2 = Now;
    const core::nanoseconds_t capt_ts3 = Now + NsPerPacket;

    write_packet(queue, new_packet(encoder, ts1, 0.11f, capt_ts1));
    write_packet(queue, new_packet(encoder, ts3, 0.33f, capt_ts3));

    expect_output(dp, SamplesPerPacket, 0.11f, capt_ts1);
    expect_output(dp, SamplesPerPacket, 0.000f, capt_ts2);
    expect_output(dp, SamplesPerPacket, 0.33f, capt_ts3);
}

TEST(depacketizer, zeros_after_packet) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    CHECK(SamplesPerPacket % 2 == 0);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));

    const size_t frame_ch = frame_spec.num_channels();

    const size_t sz1 = SamplesPerPacket / 2;
    const size_t sz2 = SamplesPerPacket;

    FramePtr f1 = frame_factory.allocate_frame_no_buffer();
    FramePtr f2 = frame_factory.allocate_frame_no_buffer();

    LONGS_EQUAL(status::StatusOK, dp.read(*f1, sz1));
    LONGS_EQUAL(status::StatusOK, dp.read(*f2, sz2));

    UNSIGNED_LONGS_EQUAL(sz1 * frame_ch, f1->num_raw_samples());
    UNSIGNED_LONGS_EQUAL(sz2 * frame_ch, f2->num_raw_samples());

    expect_values(f1->raw_samples(), SamplesPerPacket / 2 * frame_ch, 0.11f);
    expect_values(f2->raw_samples(), SamplesPerPacket / 2 * frame_ch, 0.11f);
    expect_values(f2->raw_samples() + SamplesPerPacket / 2 * frame_ch,
                  SamplesPerPacket / 2 * frame_ch, 0.00f);
}

TEST(depacketizer, packet_after_zeros) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    expect_output(dp, SamplesPerPacket, 0.00f, 0);

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
}

TEST(depacketizer, overlapping_packets) {
    CHECK(SamplesPerPacket % 2 == 0);

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const packet::stream_timestamp_t ts1 = 0;
    const packet::stream_timestamp_t ts2 = SamplesPerPacket / 2;
    const packet::stream_timestamp_t ts3 = SamplesPerPacket;

    const core::nanoseconds_t capt_ts1 = Now;
    const core::nanoseconds_t capt_ts2 = Now + NsPerPacket / 2;
    const core::nanoseconds_t capt_ts3 = Now + NsPerPacket;

    write_packet(queue, new_packet(encoder, ts1, 0.11f, capt_ts1));
    write_packet(queue, new_packet(encoder, ts2, 0.22f, capt_ts2));
    write_packet(queue, new_packet(encoder, ts3, 0.33f, capt_ts3));

    expect_output(dp, SamplesPerPacket, 0.11f, Now);
    expect_output(dp, SamplesPerPacket / 2, 0.22f, Now + NsPerPacket);
    expect_output(dp, SamplesPerPacket / 2, 0.33f, Now + NsPerPacket * 3 / 2);
}

TEST(depacketizer, frame_flags_incompltete_blank) {
    enum { PacketsPerFrame = 3 };

    PcmEncoder encoder(packet_spec);

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
        Frame::HasSignal,
        Frame::HasHoles | Frame::HasSignal,
        Frame::HasHoles | Frame::HasSignal,
        Frame::HasHoles | Frame::HasSignal,
        Frame::HasHoles | Frame::HasSignal,
        Frame::HasHoles,
        Frame::HasSignal,
        Frame::HasHoles,
    };

    core::nanoseconds_t capt_ts[] = {
        Now, Now + NsPerPacket, Now, Now, Now + NsPerPacket, 0, Now, 0,
    };

    CHECK(ROC_ARRAY_SIZE(packets) == ROC_ARRAY_SIZE(frame_flags));

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        PcmDecoder decoder(packet_spec);
        Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
        LONGS_EQUAL(status::StatusOK, dp.init_status());

        for (size_t p = 0; p < PacketsPerFrame; p++) {
            if (packets[n][p] != NULL) {
                write_packet(queue, packets[n][p]);
            }
        }

        expect_flags(dp, SamplesPerPacket * PacketsPerFrame, frame_flags[n], capt_ts[n]);
    }
}

TEST(depacketizer, frame_flags_drops) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

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
        Frame::HasSignal,                         //
        Frame::HasSignal | Frame::HasPacketDrops, //
        Frame::HasSignal,                         //
        Frame::HasHoles | Frame::HasPacketDrops,  //
        Frame::HasSignal,                         //
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        write_packet(queue, packets[n]);
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

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t capt_ts = 0;
    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f, 0);
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerFrame);

        CHECK(!dp.is_started());
        UNSIGNED_LONGS_EQUAL(0, dp.next_timestamp());
    }

    capt_ts = Now;
    for (size_t n = 0; n < NumPackets; n++) {
        const size_t nsamples = packet::stream_timestamp_t(n * SamplesPerPacket);
        write_packet(queue,
                     new_packet(encoder, StartTimestamp + nsamples, 0.1f, capt_ts));
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerPacket);
    }

    packet::stream_timestamp_t ts = StartTimestamp;

    capt_ts = Now;
    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.1f, capt_ts);
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerFrame);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.next_timestamp());
    }

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(dp, SamplesPerFrame, 0.0f, capt_ts);
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerFrame);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.next_timestamp());
    }
}

TEST(depacketizer, timestamp_fract_frame_per_packet) {
    enum {
        StartTimestamp = 1000,
        NumPackets = 3,
        SamplesPerFrame = SamplesPerPacket + 50
    };

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t capt_ts =
        Now + frame_spec.samples_per_chan_2_ns(SamplesPerPacket);
    // 1st packet in the frame has 0 capture ts, and the next
    write_packet(queue, new_packet(encoder, StartTimestamp, 0.1f, 0));
    write_packet(queue,
                 new_packet(encoder, StartTimestamp + SamplesPerPacket, 0.1f, capt_ts));

    expect_output(dp, SamplesPerFrame, 0.1f, Now);
}

TEST(depacketizer, timestamp_small_non_zero_cts) {
    enum {
        StartTimestamp = 1000,
        StartCts = 5, // very close to unix epoch
        PacketsPerFrame = 10
    };

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    // 1st packet in frame has 0 capture ts
    packet::stream_timestamp_t stream_ts = StartTimestamp;
    write_packet(queue, new_packet(encoder, StartTimestamp, 0.1f, 0));
    stream_ts += SamplesPerPacket;

    // starting from 2nd packet, there is CTS, but it starts from very
    // small value (close to unix epoch)
    core::nanoseconds_t capt_ts = StartCts;
    for (size_t n = 1; n < PacketsPerFrame; n++) {
        write_packet(queue, new_packet(encoder, stream_ts, 0.1f, capt_ts));
        stream_ts += SamplesPerPacket;
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerPacket);
    }

    // remember cts that should be used for second frame
    const core::nanoseconds_t second_frame_capt_ts = capt_ts;

    // second frame
    for (size_t n = 0; n < PacketsPerFrame; n++) {
        write_packet(queue, new_packet(encoder, stream_ts, 0.2f, capt_ts));
        stream_ts += SamplesPerPacket;
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerPacket);
    }

    // first frame has zero cts
    // if depacketizer couldn't handle small cts properly, it would
    // produce negative cts instead
    expect_output(dp, SamplesPerPacket * PacketsPerFrame, 0.1f, 0);

    // second frame has non-zero cts
    expect_output(dp, SamplesPerPacket * PacketsPerFrame, 0.2f, second_frame_capt_ts);
}

// Request big frame.
// Duration is capped so that both output frame could fit max size.
TEST(depacketizer, partial_on_big_read) {
    enum {
        // maximum # of samples that can fit one frame
        MaxFrameSamples = MaxBufSize / sizeof(sample_t) / NumCh,
        // # of frames to generate
        NumFrames = 5,
        // # of packets to fill given # of frames
        NumPackets = (MaxFrameSamples / SamplesPerPacket) * NumFrames
    };

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t pkt_cts = Now;
    for (packet::stream_timestamp_t n = 0; n < NumPackets; n++) {
        write_packet(queue, new_packet(encoder, n * SamplesPerPacket, 0.11f, pkt_cts));
        pkt_cts += frame_spec.samples_per_chan_2_ns(SamplesPerPacket);
    }

    core::nanoseconds_t frm_cts = Now;
    for (int n = 0; n < 1; n++) {
        expect_partial(dp, MaxFrameSamples * NumFrames, MaxFrameSamples, 0.11f, frm_cts);
        frm_cts += frame_spec.samples_per_chan_2_ns(MaxFrameSamples);
    }
}

// Forward error from packet reader.
TEST(depacketizer, forward_error) {
    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    packet::Queue queue;
    StatusReader reader(queue);
    Depacketizer dp(reader, decoder, frame_factory, frame_spec, false);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    // push one packet
    write_packet(queue, new_packet(encoder, 0, 0.11f, 0));

    // read first half of packet
    expect_output(dp, SamplesPerPacket / 2, 0.11f, 0);

    // packet reader will now return error
    reader.set_status(status::StatusAbort);

    // read second half of packet
    // no error because depacketizer still has buffered packet
    expect_output(dp, SamplesPerPacket / 2, 0.11f, 0);

    // try to read more
    // get error because depacketizer tries to read packet
    expect_error(dp, SamplesPerPacket, status::StatusAbort);
}

// Attach to frame pre-allocated buffers of different sizes before reading.
TEST(depacketizer, preallocated_buffer) {
    enum { FrameSz = MaxBufSize / 10 };

    const size_t buffer_list[] = {
        FrameSz * 50, // big size (reader should use it)
        FrameSz,      // exact size (reader should use it)
        FrameSz - 1,  // small size (reader should replace buffer)
        0,            // no buffer (reader should allocate buffer)
    };

    PcmEncoder encoder(packet_spec);
    PcmDecoder decoder(packet_spec);

    for (size_t bn = 0; bn < ROC_ARRAY_SIZE(buffer_list); bn++) {
        const size_t orig_buf_sz = buffer_list[bn];

        packet::Queue queue;
        StatusReader reader(queue);
        Depacketizer dp(reader, decoder, frame_factory, frame_spec, false);
        LONGS_EQUAL(status::StatusOK, dp.init_status());

        FrameFactory mock_factory(arena, orig_buf_sz * sizeof(sample_t));
        FramePtr frame = orig_buf_sz > 0 ? mock_factory.allocate_frame(0)
                                         : mock_factory.allocate_frame_no_buffer();

        core::Slice<uint8_t> orig_buf = frame->buffer();

        LONGS_EQUAL(status::StatusOK,
                    dp.read(*frame, FrameSz / frame_spec.num_channels()));

        CHECK(frame->buffer());

        if (orig_buf_sz >= FrameSz) {
            CHECK(frame->buffer() == orig_buf);
        } else {
            CHECK(frame->buffer() != orig_buf);
        }

        LONGS_EQUAL(FrameSz / frame_spec.num_channels(), frame->duration());
        LONGS_EQUAL(FrameSz, frame->num_raw_samples());
        LONGS_EQUAL(FrameSz * sizeof(sample_t), frame->num_bytes());
    }
}

} // namespace audio
} // namespace roc
