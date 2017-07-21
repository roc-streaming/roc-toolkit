/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/idecoder.h"
#include "roc_audio/iencoder.h"
#include "roc_audio/packetizer.h"
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

const sample_t Epsilon = 0.00001f;

enum {
    SamplesPerPacket = 200,

    MaxBufSize = 4096,
    MaxPackets = 100,

    NumCh = 2,
    ChMask = 0x3,

    PayloadType = 123
};

core::HeapAllocator allocator;
core::BufferPool<sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, 1);
packet::PacketPool packet_pool(allocator, 1);

rtp::Composer rtp_composer(NULL);

rtp::PCMEncoder<int16_t, NumCh> pcm_encoder;
rtp::PCMDecoder<int16_t, NumCh> pcm_decoder;

sample_t nth_sample(uint8_t n) {
    return sample_t(n) / sample_t(1 << 8);
}

class PacketChecker {
public:
    PacketChecker()
        : pos_(0)
        , src_(0)
        , sn_(0)
        , ts_(0)
        , value_(0) {
    }

    void next(packet::PacketPtr pp, size_t padding) {
        CHECK(pp);

        UNSIGNED_LONGS_EQUAL(packet::Packet::FlagRTP | packet::Packet::FlagAudio,
                             pp->flags());

        if (pos_ == 0) {
            src_ = pp->rtp()->source;
            sn_ = pp->rtp()->seqnum;
            ts_ = pp->rtp()->timestamp;
        } else {
            UNSIGNED_LONGS_EQUAL(src_, pp->rtp()->source);
            UNSIGNED_LONGS_EQUAL(sn_, pp->rtp()->seqnum);
            UNSIGNED_LONGS_EQUAL(ts_, pp->rtp()->timestamp);
        }

        UNSIGNED_LONGS_EQUAL(PayloadType, pp->rtp()->payload_type);

        CHECK(pp->rtp()->header);
        CHECK(pp->rtp()->payload);

        sample_t samples[SamplesPerPacket * NumCh] = {};

        UNSIGNED_LONGS_EQUAL(
            SamplesPerPacket,
            pcm_decoder.read_samples(*pp, 0, samples, SamplesPerPacket, ChMask));

        size_t n = 0;

        for (; n < SamplesPerPacket - padding; n++) {
            for (size_t c = 0; c < NumCh; c++) {
                DOUBLES_EQUAL(nth_sample(value_), samples[n * NumCh + c], Epsilon);
                value_++;
            }
        }

        for (; n < SamplesPerPacket; n++) {
            for (size_t c = 0; c < NumCh; c++) {
                DOUBLES_EQUAL(0, samples[n * NumCh + c], Epsilon);
            }
        }

        pos_++;
        sn_++;
        ts_ += SamplesPerPacket;
    }

private:
    size_t pos_;

    packet::source_t src_;
    packet::seqnum_t sn_;
    packet::timestamp_t ts_;

    uint8_t value_;
};

class FrameMaker {
public:
    FrameMaker()
        : value_(0) {
    }

    Frame next(size_t num_samples) {
        core::Slice<sample_t> buf =
            new (sample_buffer_pool) core::Buffer<sample_t>(sample_buffer_pool);
        CHECK(buf);

        buf.resize(num_samples * NumCh);

        for (size_t n = 0; n < num_samples; n++) {
            for (size_t c = 0; c < NumCh; c++) {
                buf.data()[n * NumCh + c] = nth_sample(value_);
                value_++;
            }
        }

        Frame frame;
        frame.samples = buf;

        return frame;
    }

private:
    uint8_t value_;
};

} // namespace

TEST_GROUP(packetizer){};

TEST(packetizer, one_buffer_one_packet) {
    enum { NumFrames = 10 };

    packet::ConcurrentQueue packet_queue(0, false);

    Packetizer packetizer(packet_queue, rtp_composer, pcm_encoder, packet_pool,
                          byte_buffer_pool, ChMask, SamplesPerPacket, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker;

    for (size_t fn = 0; fn < NumFrames; fn++) {
        UNSIGNED_LONGS_EQUAL(0, packet_queue.size());

        Frame frame = frame_maker.next(SamplesPerPacket);
        packetizer.write(frame);

        UNSIGNED_LONGS_EQUAL(1, packet_queue.size());

        packet_checker.next(packet_queue.read(), 0);
    }
}

TEST(packetizer, one_buffer_multiple_packets) {
    enum { NumPackets = 10 };

    packet::ConcurrentQueue packet_queue(0, false);

    Packetizer packetizer(packet_queue, rtp_composer, pcm_encoder, packet_pool,
                          byte_buffer_pool, ChMask, SamplesPerPacket, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker;

    Frame frame = frame_maker.next(SamplesPerPacket * NumPackets);
    packetizer.write(frame);

    for (size_t pn = 0; pn < NumPackets; pn++) {
        packet_checker.next(packet_queue.read(), 0);
    }

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

TEST(packetizer, multiple_buffers_one_packet) {
    enum { NumPackets = 10, FramesPerPacket = 4 };

    CHECK(SamplesPerPacket % FramesPerPacket == 0);

    packet::ConcurrentQueue packet_queue(0, false);

    Packetizer packetizer(packet_queue, rtp_composer, pcm_encoder, packet_pool,
                          byte_buffer_pool, ChMask, SamplesPerPacket, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker;

    for (size_t pn = 0; pn < NumPackets; pn++) {
        for (size_t fn = 0; fn < FramesPerPacket; fn++) {
            UNSIGNED_LONGS_EQUAL(0, packet_queue.size());

            Frame frame = frame_maker.next(SamplesPerPacket / FramesPerPacket);
            packetizer.write(frame);
        }

        UNSIGNED_LONGS_EQUAL(1, packet_queue.size());

        packet_checker.next(packet_queue.read(), 0);
    }
}

TEST(packetizer, multiple_buffers_multiple_packets) {
    enum {
        NumFrames = 10,
        NumSamples = (SamplesPerPacket - 1),
        NumPackets = (NumSamples * NumFrames / SamplesPerPacket)
    };

    packet::ConcurrentQueue packet_queue(0, false);

    Packetizer packetizer(packet_queue, rtp_composer, pcm_encoder, packet_pool,
                          byte_buffer_pool, ChMask, SamplesPerPacket, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker;

    for (size_t fn = 0; fn < NumFrames; fn++) {
        Frame frame = frame_maker.next(NumSamples);
        packetizer.write(frame);
    }

    for (size_t pn = 0; pn < NumPackets; pn++) {
        packet_checker.next(packet_queue.read(), 0);
    }

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

TEST(packetizer, flush) {
    enum { Padding = 10 };

    packet::ConcurrentQueue packet_queue(0, false);

    Packetizer packetizer(packet_queue, rtp_composer, pcm_encoder, packet_pool,
                          byte_buffer_pool, ChMask, SamplesPerPacket, PayloadType);

    FrameMaker frame_maker;
    PacketChecker packet_checker;

    Frame frame;

    frame = frame_maker.next(SamplesPerPacket);
    packetizer.write(frame);

    frame = frame_maker.next(SamplesPerPacket);
    packetizer.write(frame);

    frame = frame_maker.next(SamplesPerPacket - Padding);
    packetizer.write(frame);

    UNSIGNED_LONGS_EQUAL(2, packet_queue.size());

    packet_checker.next(packet_queue.read(), 0);
    packet_checker.next(packet_queue.read(), 0);

    packetizer.flush();

    packet_checker.next(packet_queue.read(), Padding);

    UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
}

} // namespace audio
} // namespace roc
