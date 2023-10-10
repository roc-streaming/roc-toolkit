/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.00001;

enum {
    SamplesPerPacket = 200,
    SampleRate = 1000,

    MaxPackets = 100,
    MaxBufSize = 4000,

    NumCh = 2,
    ChMask = 0x3,

    PayloadType = 123
};

const core::nanoseconds_t PacketDuration = SamplesPerPacket * core::Second / SampleRate;
const core::nanoseconds_t Now = 1691499037871419405;

const SampleSpec SampleSpecs(SampleRate, ChanLayout_Surround, ChanOrder_Smpte, ChMask);
const PcmFormat PcmFmt(PcmEncoding_SInt16, PcmEndian_Big);

core::HeapArena arena;
core::BufferFactory<sample_t> sample_buffer_factory(arena, MaxBufSize);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, MaxBufSize);
packet::PacketFactory packet_factory(arena);

rtp::Composer rtp_composer(NULL);

sample_t nth_sample(uint8_t n) {
    return sample_t(n) / sample_t(1 << 8);
}

class PacketChecker {
public:
    PacketChecker(IFrameDecoder& payload_decoder, core::nanoseconds_t capture_ts = Now)
        : payload_decoder_(payload_decoder)
        , pos_(0)
        , src_(0)
        , sn_(0)
        , ts_(0)
        , capture_ts_(capture_ts)
        , value_(0) {
    }

    void read(packet::IReader& reader, size_t n_samples) {
        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, reader.read(pp));
        CHECK(pp);

        UNSIGNED_LONGS_EQUAL(packet::Packet::FlagRTP | packet::Packet::FlagAudio,
                             pp->flags());

        if (pos_ == 0) {
            src_ = pp->rtp()->source;
            sn_ = pp->rtp()->seqnum;
            ts_ = pp->rtp()->stream_timestamp;
        } else {
            UNSIGNED_LONGS_EQUAL(src_, pp->rtp()->source);
            UNSIGNED_LONGS_EQUAL(sn_, pp->rtp()->seqnum);
            UNSIGNED_LONGS_EQUAL(ts_, pp->rtp()->stream_timestamp);
        }
        CHECK(core::ns_equal_delta(pp->rtp()->capture_timestamp, capture_ts_,
                                   core::Microsecond));
        capture_ts_ += SampleSpecs.samples_per_chan_2_ns(n_samples);
        UNSIGNED_LONGS_EQUAL(n_samples, pp->rtp()->duration);
        UNSIGNED_LONGS_EQUAL(PayloadType, pp->rtp()->payload_type);

        CHECK(pp->rtp()->header);
        CHECK(pp->rtp()->payload);

        payload_decoder_.begin(pp->rtp()->stream_timestamp, pp->rtp()->payload.data(),
                               pp->rtp()->payload.size());

        sample_t samples[SamplesPerPacket * NumCh] = {};

        UNSIGNED_LONGS_EQUAL(n_samples, payload_decoder_.read(samples, SamplesPerPacket));

        payload_decoder_.end();

        size_t n = 0;

        for (; n < n_samples; n++) {
            for (size_t c = 0; c < NumCh; c++) {
                DOUBLES_EQUAL((double)nth_sample(value_), (double)samples[n * NumCh + c],
                              Epsilon);
                value_++;
            }
        }

        pos_++;
        sn_++;
        ts_ += n_samples;
    }

private:
    IFrameDecoder& payload_decoder_;

    size_t pos_;

    packet::stream_source_t src_;
    packet::seqnum_t sn_;
    packet::stream_timestamp_t ts_;
    core::nanoseconds_t capture_ts_;

    uint8_t value_;
};

class FrameMaker {
public:
    FrameMaker(core::nanoseconds_t capture_ts = Now)
        : value_(0)
        , capture_ts_(capture_ts) {
    }

    void write(IFrameWriter& writer, size_t num_samples) {
        core::Slice<sample_t> buf = sample_buffer_factory.new_buffer();
        CHECK(buf);

        buf.reslice(0, num_samples * NumCh);

        for (size_t n = 0; n < num_samples; n++) {
            for (size_t c = 0; c < NumCh; c++) {
                buf.data()[n * NumCh + c] = nth_sample(value_);
                value_++;
            }
        }

        Frame frame(buf.data(), buf.size());
        frame.set_capture_timestamp(capture_ts_);
        capture_ts_ += SampleSpecs.samples_per_chan_2_ns(num_samples);
        writer.write(frame);
    }

private:
    uint8_t value_;
    core::nanoseconds_t capture_ts_;
};

} // namespace

TEST_GROUP(packetizer) {};

TEST(packetizer, one_buffer_one_packet) {
    enum { NumFrames = 10 };

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue packet_queue;

    Packetizer packetizer(packet_queue, rtp_composer, encoder, packet_factory,
                          byte_buffer_factory, PacketDuration, SampleSpecs, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t fn = 0; fn < NumFrames; fn++) {
        UNSIGNED_LONGS_EQUAL(0, packet_queue.size());

        frame_maker.write(packetizer, SamplesPerPacket);

        UNSIGNED_LONGS_EQUAL(1, packet_queue.size());

        packet_checker.read(packet_queue, SamplesPerPacket);
    }
}

TEST(packetizer, one_buffer_multiple_packets) {
    enum { NumPackets = 10 };

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue packet_queue;

    Packetizer packetizer(packet_queue, rtp_composer, encoder, packet_factory,
                          byte_buffer_factory, PacketDuration, SampleSpecs, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    frame_maker.write(packetizer, SamplesPerPacket * NumPackets);

    for (size_t pn = 0; pn < NumPackets; pn++) {
        packet_checker.read(packet_queue, SamplesPerPacket);
    }

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

TEST(packetizer, multiple_buffers_one_packet) {
    enum { NumPackets = 10, FramesPerPacket = 4 };

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue packet_queue;

    Packetizer packetizer(packet_queue, rtp_composer, encoder, packet_factory,
                          byte_buffer_factory, PacketDuration, SampleSpecs, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t pn = 0; pn < NumPackets; pn++) {
        for (size_t fn = 0; fn < FramesPerPacket; fn++) {
            UNSIGNED_LONGS_EQUAL(0, packet_queue.size());

            frame_maker.write(packetizer, SamplesPerPacket / FramesPerPacket);
        }

        UNSIGNED_LONGS_EQUAL(1, packet_queue.size());

        packet_checker.read(packet_queue, SamplesPerPacket);
    }
}

TEST(packetizer, multiple_buffers_multiple_packets) {
    enum {
        NumFrames = 10,
        NumSamples = (SamplesPerPacket - 1),
        NumPackets = (NumSamples * NumFrames / SamplesPerPacket)
    };

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue packet_queue;

    Packetizer packetizer(packet_queue, rtp_composer, encoder, packet_factory,
                          byte_buffer_factory, PacketDuration, SampleSpecs, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t fn = 0; fn < NumFrames; fn++) {
        frame_maker.write(packetizer, NumSamples);
    }

    for (size_t pn = 0; pn < NumPackets; pn++) {
        packet_checker.read(packet_queue, SamplesPerPacket);
    }

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

TEST(packetizer, flush) {
    enum { NumIterations = 5, Missing = 10 };

    PcmEncoder encoder(PcmFmt, SampleSpecs);
    PcmDecoder decoder(PcmFmt, SampleSpecs);

    packet::Queue packet_queue;

    Packetizer packetizer(packet_queue, rtp_composer, encoder, packet_factory,
                          byte_buffer_factory, PacketDuration, SampleSpecs, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker(decoder);

    for (size_t n = 0; n < NumIterations; n++) {
        frame_maker.write(packetizer, SamplesPerPacket);
        frame_maker.write(packetizer, SamplesPerPacket);
        frame_maker.write(packetizer, SamplesPerPacket - Missing);

        UNSIGNED_LONGS_EQUAL(2, packet_queue.size());

        packet_checker.read(packet_queue, SamplesPerPacket);
        packet_checker.read(packet_queue, SamplesPerPacket);

        packetizer.flush();

        packet_checker.read(packet_queue, SamplesPerPacket - Missing);

        UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
    }
}

} // namespace audio
} // namespace roc
