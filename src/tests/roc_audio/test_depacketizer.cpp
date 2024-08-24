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
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
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
    SampleRate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

const SampleSpec packet_spec(
    SampleRate, PcmSubformat_SInt16_Be, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

const core::nanoseconds_t NsPerPacket =
    packet_spec.samples_per_chan_2_ns(SamplesPerPacket);
const core::nanoseconds_t Now = 1691499037871419405;

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, MaxBufSize);
FrameFactory frame_factory(arena, MaxBufSize);

rtp::Composer rtp_composer(NULL, arena);

packet::PacketPtr new_packet(IFrameEncoder& encoder,
                             packet::stream_timestamp_t ts,
                             sample_t value,
                             core::nanoseconds_t capt_ts = 0) {
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

    encoder.begin_frame(pp->rtp()->payload.data(), pp->rtp()->payload.size());

    UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                         encoder.write_samples(samples, SamplesPerPacket));

    encoder.end_frame();

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

void expect_output(status::StatusCode expected_code,
                   Depacketizer& depacketizer,
                   size_t requested_samples_per_chan,
                   size_t expected_samples_per_chan,
                   sample_t value,
                   core::nanoseconds_t capt_ts = -1,
                   int flags = -1,
                   FrameReadMode mode = ModeHard) {
    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(expected_code,
                depacketizer.read(*frame,
                                  (packet::stream_timestamp_t)requested_samples_per_chan,
                                  mode));

    CHECK(frame->is_raw());

    UNSIGNED_LONGS_EQUAL(expected_samples_per_chan, frame->duration());
    UNSIGNED_LONGS_EQUAL(expected_samples_per_chan * frame_spec.num_channels(),
                         frame->num_raw_samples());

    if (flags != -1) {
        LONGS_EQUAL(flags, frame->flags());
    }

    if (capt_ts != -1) {
        CHECK(
            core::ns_equal_delta(frame->capture_timestamp(), capt_ts, core::Microsecond));
    }

    expect_values(frame->raw_samples(),
                  expected_samples_per_chan * frame_spec.num_channels(), value);
}

void expect_error(status::StatusCode expected_status,
                  Depacketizer& depacketizer,
                  size_t requested_samples_per_chan,
                  FrameReadMode mode = ModeHard) {
    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(expected_status,
                depacketizer.read(*frame,
                                  (packet::stream_timestamp_t)requested_samples_per_chan,
                                  mode));
}

void expect_n_decoded(int packet_count, Depacketizer& depacketizer) {
    LONGS_EQUAL(packet_count, depacketizer.metrics().decoded_packets);
}

void expect_n_late(int packet_count, Depacketizer& depacketizer) {
    LONGS_EQUAL(packet_count, depacketizer.metrics().late_packets);
}

class ArrayReader : public packet::IReader {
public:
    ArrayReader()
        : next_index_(0) {
    }

    virtual status::StatusCode read(packet::PacketPtr& pp, packet::PacketReadMode mode) {
        for (size_t index = next_index_; index < MaxPackets; index++) {
            pp = packets_[index];
            if (pp) {
                if (mode == packet::ModeFetch) {
                    next_index_ = index + 1;
                }
                return status::StatusOK;
            }
        }
        return status::StatusDrain;
    }

    size_t num_packets() {
        size_t count = 0;
        for (size_t index = next_index_; index < MaxPackets; index++) {
            if (packets_[index]) {
                count++;
            }
        }
        return count;
    }

    void set_packet(size_t index, const packet::PacketPtr& packet) {
        CHECK(index < MaxPackets);
        CHECK(index >= next_index_);
        packets_[index] = packet;
    }

private:
    enum { MaxPackets = 20 };

    packet::PacketPtr packets_[MaxPackets];
    size_t next_index_;
};

class StatusReader : public packet::IReader {
public:
    explicit StatusReader(packet::IReader& reader)
        : reader_(reader)
        , code_(status::NoStatus) {
    }

    virtual status::StatusCode read(packet::PacketPtr& pp, packet::PacketReadMode mode) {
        if (code_ != status::NoStatus && code_ != status::StatusOK) {
            return code_;
        }
        return reader_.read(pp, mode);
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

// Frame size same as packet size.
TEST(depacketizer, one_packet_one_read) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f, Now);

    expect_n_decoded(1, dp);
    expect_n_late(0, dp);
}

// Small frame, big packet.
TEST(depacketizer, one_packet_multiple_reads) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));

    LONGS_EQUAL(1, queue.size());

    core::nanoseconds_t cts = Now;
    for (size_t n = 0; n < SamplesPerPacket; n++) {
        expect_output(status::StatusOK, dp, 1, 1, 0.11f, cts);
        cts += frame_spec.samples_per_chan_2_ns(1);

        LONGS_EQUAL(0, queue.size());
    }

    expect_n_decoded(1, dp);
    expect_n_late(0, dp);
}

// Big frame, small packets.
TEST(depacketizer, multiple_packets_one_read) {
    enum { NumPackets = 10 };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t cts = Now;
    for (packet::stream_timestamp_t n = 0; n < NumPackets; n++) {
        write_packet(queue, new_packet(encoder, n * SamplesPerPacket, 0.11f, cts));
        cts += NsPerPacket;

        LONGS_EQUAL(n + 1, queue.size());
    }

    expect_output(status::StatusOK, dp, NumPackets * SamplesPerPacket,
                  NumPackets * SamplesPerPacket, 0.11f, Now);

    LONGS_EQUAL(0, queue.size());

    expect_n_decoded(NumPackets, dp);
    expect_n_late(0, dp);
}

TEST(depacketizer, multiple_packets_multiple_reads) {
    enum { FramesPerPacket = 10 };

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    // Start with a packet with zero capture timestamp.
    write_packet(queue, new_packet(encoder, 0, 0.01f, 0));
    const size_t samples_per_frame = SamplesPerPacket / FramesPerPacket;
    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(status::StatusOK, dp, samples_per_frame, samples_per_frame, 0.01f,
                      0);
    }
    LONGS_EQUAL(0, queue.size());

    {
        core::nanoseconds_t cts = Now;
        write_packet(queue, new_packet(encoder, 1 * SamplesPerPacket, 0.11f, cts));
        cts += NsPerPacket;
        write_packet(queue, new_packet(encoder, 2 * SamplesPerPacket, 0.22f, cts));
        cts += NsPerPacket;
        write_packet(queue, new_packet(encoder, 3 * SamplesPerPacket, 0.33f, cts));
        LONGS_EQUAL(3, queue.size());
    }

    core::nanoseconds_t cts = Now;

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(status::StatusOK, dp, samples_per_frame, samples_per_frame, 0.11f,
                      cts);
        cts += frame_spec.samples_per_chan_2_ns(samples_per_frame);
    }
    LONGS_EQUAL(2, queue.size());

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(status::StatusOK, dp, samples_per_frame, samples_per_frame, 0.22f,
                      cts);
        cts += frame_spec.samples_per_chan_2_ns(samples_per_frame);
    }
    LONGS_EQUAL(1, queue.size());

    for (size_t n = 0; n < FramesPerPacket; n++) {
        expect_output(status::StatusOK, dp, samples_per_frame, samples_per_frame, 0.33f,
                      cts);
        cts += frame_spec.samples_per_chan_2_ns(samples_per_frame);
    }
    LONGS_EQUAL(0, queue.size());
}

// Wrapping of 32-bit packet stream timestamp (STS).
TEST(depacketizer, timestamp_wrap) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const packet::stream_timestamp_t ts2 = 0;
    const packet::stream_timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts2 + SamplesPerPacket;

    {
        core::nanoseconds_t cts = Now;
        write_packet(queue, new_packet(encoder, ts1, 0.11f, cts));
        cts += NsPerPacket;
        write_packet(queue, new_packet(encoder, ts2, 0.22f, cts));
        cts += NsPerPacket;
        write_packet(queue, new_packet(encoder, ts3, 0.33f, cts));
        LONGS_EQUAL(3, queue.size());
    }

    core::nanoseconds_t cts = Now;
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f, cts);
    cts += NsPerPacket;
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.22f, cts);
    cts += NsPerPacket;
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.33f, cts);
    LONGS_EQUAL(0, queue.size());
}

TEST(depacketizer, drop_late_packets) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
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
    LONGS_EQUAL(3, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f,
                  capt_ts1);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.33f,
                  capt_ts3);
    LONGS_EQUAL(0, queue.size());

    // 2 packets decoded, 1 dropped
    expect_n_decoded(2, dp);
    expect_n_late(1, dp);
}

TEST(depacketizer, drop_late_packets_timestamp_wrap) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
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
    LONGS_EQUAL(3, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f,
                  capt_ts1);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.33f,
                  capt_ts3);
    LONGS_EQUAL(0, queue.size());

    // 2 packets decoded, 1 dropped
    expect_n_decoded(2, dp);
    expect_n_late(1, dp);
}

TEST(depacketizer, zeros_no_first_packet) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f, 0);
}

TEST(depacketizer, zeros_no_next_packet) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, 0));
    LONGS_EQUAL(1, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f, 0);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  0); // no packet -- no cts
    LONGS_EQUAL(0, queue.size());
}

TEST(depacketizer, zeros_between_packets) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const core::nanoseconds_t capt_ts1 = Now;
    const core::nanoseconds_t capt_ts2 = Now + NsPerPacket * 2;

    write_packet(queue, new_packet(encoder, 1 * SamplesPerPacket, 0.11f, capt_ts1));
    write_packet(queue, new_packet(encoder, 3 * SamplesPerPacket, 0.33f, capt_ts2));
    LONGS_EQUAL(2, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f, Now);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  Now + NsPerPacket);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.33f,
                  Now + 2 * NsPerPacket);
    LONGS_EQUAL(0, queue.size());
}

TEST(depacketizer, zeros_between_packets_timestamp_wrap) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    const packet::stream_timestamp_t ts2 = 0;
    const packet::stream_timestamp_t ts1 = ts2 - SamplesPerPacket;
    const packet::stream_timestamp_t ts3 = ts2 + SamplesPerPacket;
    const core::nanoseconds_t capt_ts1 = Now - NsPerPacket;
    const core::nanoseconds_t capt_ts2 = Now;
    const core::nanoseconds_t capt_ts3 = Now + NsPerPacket;

    write_packet(queue, new_packet(encoder, ts1, 0.11f, capt_ts1));
    write_packet(queue, new_packet(encoder, ts3, 0.33f, capt_ts3));
    LONGS_EQUAL(2, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f,
                  capt_ts1);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  capt_ts2);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.33f,
                  capt_ts3);
    LONGS_EQUAL(0, queue.size());
}

TEST(depacketizer, zeros_after_packet) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    CHECK(SamplesPerPacket % 2 == 0);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));
    LONGS_EQUAL(1, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket / 2, SamplesPerPacket / 2, 0.11f,
                  Now);
    expect_output(status::StatusPart, dp, SamplesPerPacket, SamplesPerPacket / 2, 0.11f,
                  Now + NsPerPacket / 2);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  Now + NsPerPacket);
    LONGS_EQUAL(0, queue.size());
}

TEST(depacketizer, packet_after_zeros) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f, 0);

    write_packet(queue, new_packet(encoder, 0, 0.11f, Now));
    LONGS_EQUAL(1, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f, Now);
    LONGS_EQUAL(0, queue.size());
}

// Depacketizer should handle the case when new packet partially overlaps with
// previous packets. It should drop unneeded parts.
TEST(depacketizer, overlapping_packets) {
    CHECK(SamplesPerPacket % 2 == 0);

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
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
    LONGS_EQUAL(3, queue.size());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f, Now);
    expect_output(status::StatusOK, dp, SamplesPerPacket / 2, SamplesPerPacket / 2, 0.22f,
                  Now + NsPerPacket);
    expect_output(status::StatusOK, dp, SamplesPerPacket / 2, SamplesPerPacket / 2, 0.33f,
                  Now + NsPerPacket * 3 / 2);
    LONGS_EQUAL(0, queue.size());
}

// Scenario described in gh-54 and gh-210.
// Depacketizer should check what is next packet using ModePeek and don't fetch
// packet if it's not needed yet.
TEST(depacketizer, late_reordered) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    CHECK(SamplesPerPacket % 2 == 0);

    ArrayReader reader;
    Depacketizer dp(reader, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    packet::PacketPtr p1 =
        new_packet(encoder, 1 * SamplesPerPacket, 0.11f, Now + NsPerPacket * 1);
    packet::PacketPtr p2 =
        new_packet(encoder, 2 * SamplesPerPacket, 0.22f, Now + NsPerPacket * 2);
    packet::PacketPtr p3 =
        new_packet(encoder, 3 * SamplesPerPacket, 0.33f, Now + NsPerPacket * 3);
    packet::PacketPtr p4 =
        new_packet(encoder, 4 * SamplesPerPacket, 0.44f, Now + NsPerPacket * 4);

    reader.set_packet(1, p1);
    reader.set_packet(4, p4);
    LONGS_EQUAL(2, reader.num_packets());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.11f,
                  Now + NsPerPacket * 1);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  Now + NsPerPacket * 2);
    LONGS_EQUAL(1, reader.num_packets()); // p4 not fetched

    reader.set_packet(2, p2);
    reader.set_packet(3, p3);
    LONGS_EQUAL(3, reader.num_packets());

    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.33f,
                  Now + NsPerPacket * 3);
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.44f,
                  Now + NsPerPacket * 4);
    LONGS_EQUAL(0, reader.num_packets());

    // 3 packets decoded, 1 dropped
    expect_n_decoded(3, dp);
    expect_n_late(1, dp);
}

// In hard read mode, depacketizer should fill packet losses with zeros and generate
// partial reads to avoid mixing losses and normal samples in a same frame.
TEST(depacketizer, frequent_losses_hard_read) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t pkt_cts = Now;
    core::nanoseconds_t frm_cts = Now;

    // write 1, write 2, write 3
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 1, 0.11f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 2, 0.11f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 3, 0.11f, pkt_cts));
    pkt_cts += NsPerPacket;

    // read 1+2+3(signal)
    expect_output(status::StatusOK, dp, SamplesPerPacket * 3, SamplesPerPacket * 3, 0.11f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket * 3;
    LONGS_EQUAL(0, queue.size());

    // lose 4, write 5, write 6
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 5, 0.22f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 6, 0.22f, pkt_cts));
    pkt_cts += NsPerPacket;

    // read 4(gap)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    // read 5+6(signal)
    expect_output(status::StatusOK, dp, SamplesPerPacket * 2, SamplesPerPacket * 2, 0.22f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket * 2;
    LONGS_EQUAL(0, queue.size());

    // write 7, lose 8, write 9
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 7, 0.33f, pkt_cts));
    pkt_cts += NsPerPacket;
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 9, 0.33f, pkt_cts));
    pkt_cts += NsPerPacket;

    // read 7(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket, 0.33f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    // read 8(gap)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 2, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    // read 9(signal)
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.33f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    LONGS_EQUAL(0, queue.size());

    // write 10, write 11, lose 12
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 10, 0.44f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 11, 0.44f, pkt_cts));
    pkt_cts += NsPerPacket;
    pkt_cts += NsPerPacket;

    // read 10+11(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket * 2,
                  0.44f, frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket * 2;
    // read 12(gap)
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    LONGS_EQUAL(0, queue.size());

    // lose 13, write 14, lose 15
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 14, 0.55f, pkt_cts));
    pkt_cts += NsPerPacket;
    pkt_cts += NsPerPacket;

    // read 13(gap)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    // read 14(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 2, SamplesPerPacket, 0.55f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    // read 15(gap)
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    LONGS_EQUAL(0, queue.size());

    // write 16, write 17, write 18
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 16, 0.66f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 17, 0.66f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 18, 0.66f, pkt_cts));
    pkt_cts += NsPerPacket;

    // read 16+17+18(signal)
    expect_output(status::StatusOK, dp, SamplesPerPacket * 3, SamplesPerPacket * 3, 0.66f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket * 3;
    LONGS_EQUAL(0, queue.size());

    // self check
    LONGLONGS_EQUAL(pkt_cts, frm_cts);
}

// In soft read mode, depacketizer should stop reading on packet loss and
// generate partial read or drain.
TEST(depacketizer, frequent_losses_soft_read) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t pkt_cts = Now;
    core::nanoseconds_t frm_cts = Now;

    // write 1, write 2, write 3
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 1, 0.11f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 2, 0.11f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 3, 0.11f, pkt_cts));
    pkt_cts += NsPerPacket;

    // soft read drain(not started)
    expect_error(status::StatusDrain, dp, SamplesPerPacket * 3, ModeSoft);
    // hard read 1+2+3(signal)
    expect_output(status::StatusOK, dp, SamplesPerPacket * 3, SamplesPerPacket * 3, 0.11f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket * 3;
    LONGS_EQUAL(0, queue.size());

    // lose 4, write 5, write 6
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 5, 0.22f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 6, 0.22f, pkt_cts));
    pkt_cts += NsPerPacket;

    // soft read drain(gap)
    expect_error(status::StatusDrain, dp, SamplesPerPacket * 3, ModeSoft);
    // hard read 4(gap)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    // soft read 5+6(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket * 2,
                  0.22f, frm_cts, -1, ModeSoft);
    frm_cts += NsPerPacket * 2;
    LONGS_EQUAL(0, queue.size());

    // write 7, lose 8, write 9
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 7, 0.33f, pkt_cts));
    pkt_cts += NsPerPacket;
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 9, 0.33f, pkt_cts));
    pkt_cts += NsPerPacket;

    // soft read 7(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket, 0.33f,
                  frm_cts, -1, ModeSoft);
    frm_cts += NsPerPacket;
    // soft read drain(gap)
    expect_error(status::StatusDrain, dp, SamplesPerPacket * 2, ModeSoft);
    // hard read 8(gap)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 2, SamplesPerPacket, 0.00f,
                  frm_cts);
    frm_cts += NsPerPacket;
    // soft read 9(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 2, SamplesPerPacket, 0.33f,
                  frm_cts, -1, ModeSoft);
    frm_cts += NsPerPacket;
    LONGS_EQUAL(0, queue.size());

    // write 10, write 11, lose 12
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 10, 0.44f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 11, 0.44f, pkt_cts));
    pkt_cts += NsPerPacket;
    pkt_cts += NsPerPacket;

    // soft read 10+11(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket * 2,
                  0.44f, frm_cts, -1, ModeSoft);
    frm_cts += NsPerPacket * 2;
    // soft read drain(gap)
    expect_error(status::StatusDrain, dp, SamplesPerPacket, ModeSoft);
    // read 12(gap)
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    LONGS_EQUAL(0, queue.size());

    // lose 13, write 14, lose 15
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 14, 0.55f, pkt_cts));
    pkt_cts += NsPerPacket;
    pkt_cts += NsPerPacket;

    // soft read drain(gap)
    expect_error(status::StatusDrain, dp, SamplesPerPacket, ModeSoft);
    // hard read 13(gap)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 3, SamplesPerPacket, 0.00f,
                  frm_cts);
    frm_cts += NsPerPacket;
    // soft read 14(signal)
    expect_output(status::StatusPart, dp, SamplesPerPacket * 2, SamplesPerPacket, 0.55f,
                  frm_cts, -1, ModeSoft);
    frm_cts += NsPerPacket;
    // soft read drain(gap)
    expect_error(status::StatusDrain, dp, SamplesPerPacket, ModeSoft);
    // hard read 15(gap)
    expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket, 0.00f,
                  frm_cts, -1, ModeHard);
    frm_cts += NsPerPacket;
    LONGS_EQUAL(0, queue.size());

    // write 16, write 17, write 18
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 16, 0.66f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 17, 0.66f, pkt_cts));
    pkt_cts += NsPerPacket;
    write_packet(queue, new_packet(encoder, SamplesPerPacket * 18, 0.66f, pkt_cts));
    pkt_cts += NsPerPacket;

    // read 16+17+18(signal)
    expect_output(status::StatusOK, dp, SamplesPerPacket * 3, SamplesPerPacket * 3, 0.66f,
                  frm_cts, -1, ModeSoft);
    frm_cts += NsPerPacket * 3;
    LONGS_EQUAL(0, queue.size());

    // self check
    LONGLONGS_EQUAL(pkt_cts, frm_cts);
}

TEST(depacketizer, frame_flags_signal_gaps) {
    enum { PacketsPerFrame = 3 };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

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
            NULL,
            NULL,
            NULL,
        },
        {
            new_packet(encoder, SamplesPerPacket * 19, 0.11f),
            new_packet(encoder, SamplesPerPacket * 20, 0.11f),
            new_packet(encoder, SamplesPerPacket * 21, 0.11f),
        },
        {
            NULL,
            NULL,
            NULL,
        },
    };

    unsigned frames[][PacketsPerFrame][2] = {
        {
            { SamplesPerPacket * 3, Frame::HasSignal },
        },
        {
            { SamplesPerPacket * 1, Frame::HasGaps },
            { SamplesPerPacket * 2, Frame::HasSignal },
        },
        {
            { SamplesPerPacket * 1, Frame::HasSignal },
            { SamplesPerPacket * 1, Frame::HasGaps },
            { SamplesPerPacket * 1, Frame::HasSignal },
        },
        {
            { SamplesPerPacket * 2, Frame::HasSignal },
            { SamplesPerPacket * 1, Frame::HasGaps },
        },
        {
            { SamplesPerPacket * 1, Frame::HasGaps },
            { SamplesPerPacket * 1, Frame::HasSignal },
            { SamplesPerPacket * 1, Frame::HasGaps },
        },
        {
            { SamplesPerPacket * 3, Frame::HasGaps },
        },
        {
            { SamplesPerPacket * 3, Frame::HasSignal },
        },
        {
            { SamplesPerPacket * 3, Frame::HasGaps },
        },
    };

    CHECK(ROC_ARRAY_SIZE(packets) == ROC_ARRAY_SIZE(frames));

    for (size_t i = 0; i < ROC_ARRAY_SIZE(packets); i++) {
        for (size_t np = 0; np < PacketsPerFrame; np++) {
            if (packets[i][np] == NULL) {
                continue;
            }

            write_packet(queue, packets[i][np]);
        }

        size_t remain_samples = SamplesPerPacket * PacketsPerFrame;

        for (size_t nf = 0; nf < PacketsPerFrame; nf++) {
            const size_t expected_samples = frames[i][nf][0];
            if (expected_samples == 0) {
                continue;
            }

            const int expected_flags = (int)frames[i][nf][1];
            const sample_t expected_value =
                expected_flags & Frame::HasSignal ? 0.11f : 0.00f;

            const status::StatusCode expected_status = expected_samples == remain_samples
                ? status::StatusOK
                : status::StatusPart;

            expect_output(expected_status, dp, remain_samples, expected_samples,
                          expected_value, -1, expected_flags);

            remain_samples -= expected_samples;
        }

        LONGS_EQUAL(0, remain_samples);
    }
}

TEST(depacketizer, frame_flags_drops) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    packet::PacketPtr packets[] = {
        new_packet(encoder, SamplesPerPacket * 4, 0.11f),
        new_packet(encoder, SamplesPerPacket * 1, 0.11f),
        new_packet(encoder, SamplesPerPacket * 2, 0.11f),
        new_packet(encoder, SamplesPerPacket * 5, 0.11f),
        new_packet(encoder, SamplesPerPacket * 6, 0.11f),
        new_packet(encoder, SamplesPerPacket * 3, 0.11f),
        new_packet(encoder, SamplesPerPacket * 8, 0.11f),
    };

    int frames[] = {
        Frame::HasSignal,                   //
        Frame::HasSignal | Frame::HasDrops, //
        Frame::HasSignal,                   //
        Frame::HasGaps | Frame::HasDrops,   //
        Frame::HasSignal,                   //
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(packets); n++) {
        write_packet(queue, packets[n]);
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(frames); n++) {
        const int frame_flags = frames[n];
        const sample_t frame_value = frame_flags & Frame::HasGaps ? 0.00f : 0.11f;

        expect_output(status::StatusOK, dp, SamplesPerPacket, SamplesPerPacket,
                      frame_value, -1, frame_flags);
    }

    // 3 packets were late and dropped
    expect_n_late(3, dp);
}

TEST(depacketizer, capture_timestamp) {
    enum {
        StartTimestamp = 1000,
        NumPackets = 3,
        FramesPerPacket = 10,
        SamplesPerFrame = SamplesPerPacket / FramesPerPacket
    };

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t capt_ts = 0;
    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(status::StatusOK, dp, SamplesPerFrame, SamplesPerFrame, 0.0f, 0);
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
        expect_output(status::StatusOK, dp, SamplesPerFrame, SamplesPerFrame, 0.1f,
                      capt_ts);
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerFrame);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.next_timestamp());
    }

    for (size_t n = 0; n < NumPackets * FramesPerPacket; n++) {
        expect_output(status::StatusOK, dp, SamplesPerFrame, SamplesPerFrame, 0.0f,
                      capt_ts);
        capt_ts += frame_spec.samples_per_chan_2_ns(SamplesPerFrame);

        ts += SamplesPerFrame;

        CHECK(dp.is_started());
        UNSIGNED_LONGS_EQUAL(ts, dp.next_timestamp());
    }
}

TEST(depacketizer, capture_timestamp_fract_frame_per_packet) {
    enum {
        StartTimestamp = 1000,
        NumPackets = 3,
        SamplesPerFrame = SamplesPerPacket + 50
    };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t capt_ts =
        Now + frame_spec.samples_per_chan_2_ns(SamplesPerPacket);
    // 1st packet in the frame has 0 capture ts, and the next
    write_packet(queue, new_packet(encoder, StartTimestamp, 0.1f, 0));
    write_packet(queue,
                 new_packet(encoder, StartTimestamp + SamplesPerPacket, 0.1f, capt_ts));

    expect_output(status::StatusOK, dp, SamplesPerFrame, SamplesPerFrame, 0.1f, Now);
}

TEST(depacketizer, capture_timestamp_small_non_zero) {
    enum {
        StartTimestamp = 1000,
        StartCts = 5, // very close to unix epoch
        PacketsPerFrame = 10
    };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
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
    expect_output(status::StatusOK, dp, SamplesPerPacket * PacketsPerFrame,
                  SamplesPerPacket * PacketsPerFrame, 0.1f, 0);

    // second frame has non-zero cts
    expect_output(status::StatusOK, dp, SamplesPerPacket * PacketsPerFrame,
                  SamplesPerPacket * PacketsPerFrame, 0.2f, second_frame_capt_ts);
}

// Request big frame.
// Duration is capped so that output frame could fit max size.
TEST(depacketizer, partial_on_big_read) {
    enum {
        // maximum # of samples that can fit one frame
        MaxFrameSamples = MaxBufSize / sizeof(sample_t) / NumCh,
        // # of frames to generate
        NumFrames = 5,
        // # of packets to fill given # of frames
        NumPackets = (MaxFrameSamples / SamplesPerPacket) * NumFrames
    };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    Depacketizer dp(queue, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    core::nanoseconds_t pkt_cts = Now;
    for (packet::stream_timestamp_t n = 0; n < NumPackets; n++) {
        write_packet(queue, new_packet(encoder, n * SamplesPerPacket, 0.11f, pkt_cts));
        pkt_cts += frame_spec.samples_per_chan_2_ns(SamplesPerPacket);
    }

    core::nanoseconds_t frm_cts = Now;
    for (int n = 0; n < 1; n++) {
        expect_output(status::StatusPart, dp, MaxFrameSamples * NumFrames,
                      MaxFrameSamples, 0.11f, frm_cts);
        frm_cts += frame_spec.samples_per_chan_2_ns(MaxFrameSamples);
    }
}

// Forward error from packet reader.
TEST(depacketizer, forward_error) {
    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    packet::FifoQueue queue;
    StatusReader reader(queue);
    Depacketizer dp(reader, decoder, frame_factory, frame_spec, NULL);
    LONGS_EQUAL(status::StatusOK, dp.init_status());

    // push one packet
    write_packet(queue, new_packet(encoder, 0, 0.11f, 0));

    // read first half of packet
    expect_output(status::StatusOK, dp, SamplesPerPacket / 2, SamplesPerPacket / 2, 0.11f,
                  0);

    // packet reader will now return error
    reader.set_status(status::StatusAbort);

    // read second half of packet
    // no error because depacketizer still has buffered packet
    expect_output(status::StatusOK, dp, SamplesPerPacket / 2, SamplesPerPacket / 2, 0.11f,
                  0);

    // try to read more
    // get error because depacketizer tries to read packet
    expect_error(status::StatusAbort, dp, SamplesPerPacket);
}

// Attach to frame pre-allocated buffers of different sizes before reading.
TEST(depacketizer, preallocated_buffer) {
    enum { FrameSz = MaxBufSize / 10 };

    const size_t buffer_list[] = {
        FrameSz * 50, // big size (depacketizer should use it)
        FrameSz,      // exact size (depacketizer should use it)
        FrameSz - 1,  // small size (depacketizer should replace buffer)
        0,            // no buffer (depacketizer should allocate buffer)
    };

    PcmEncoder encoder(packet_spec, arena);
    PcmDecoder decoder(packet_spec, arena);

    for (size_t bn = 0; bn < ROC_ARRAY_SIZE(buffer_list); bn++) {
        const size_t orig_buf_sz = buffer_list[bn];

        packet::FifoQueue queue;
        StatusReader reader(queue);
        Depacketizer dp(reader, decoder, frame_factory, frame_spec, NULL);
        LONGS_EQUAL(status::StatusOK, dp.init_status());

        FrameFactory mock_factory(arena, orig_buf_sz * sizeof(sample_t));
        FramePtr frame = orig_buf_sz > 0 ? mock_factory.allocate_frame(0)
                                         : mock_factory.allocate_frame_no_buffer();

        core::Slice<uint8_t> orig_buf = frame->buffer();

        LONGS_EQUAL(status::StatusOK,
                    dp.read(*frame, FrameSz / frame_spec.num_channels(), ModeHard));

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
