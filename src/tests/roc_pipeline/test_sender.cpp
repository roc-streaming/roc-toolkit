/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/sender.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"
#include "roc_rtp/pcm_decoder.h"

#include "test_frame_writer.h"
#include "test_packet_reader.h"

namespace roc {
namespace pipeline {

namespace {

rtp::PayloadType PayloadType = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 4096,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    ManyFrames = FramesPerPacket * 20
};

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, 1);
packet::PacketPool packet_pool(allocator, 1);

rtp::FormatMap format_map;
rtp::Parser rtp_parser(format_map, NULL);
rtp::PCMDecoder<int16_t, NumCh> pcm_decoder;

} // namespace

TEST_GROUP(sender) {
    SenderConfig config;

    void setup() {
        config.source_port.address = new_address(1);
        config.source_port.protocol = Proto_RTP;

        config.sample_rate = SampleRate;
        config.channels = ChMask;
        config.samples_per_packet = SamplesPerPacket;

        config.interleaving = false;
        config.timing = false;
    }
};

TEST(sender, write) {
    packet::ConcurrentQueue queue(0, false);

    Sender sender(config, queue, queue, format_map, packet_pool, byte_buffer_pool,
                  allocator);

    CHECK(sender.valid());

    FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * NumCh);
    }

    PacketReader packet_reader(queue, rtp_parser, pcm_decoder, packet_pool, PayloadType,
                               config.source_port.address);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

TEST(sender, frame_size_small) {
    enum {
        SamplesPerSmallFrame = SamplesPerFrame / 2,
        SmallFramesPerPacket = SamplesPerPacket / SamplesPerSmallFrame,
        ManySmallFrames = SmallFramesPerPacket * 20
    };

    packet::ConcurrentQueue queue(0, false);

    Sender sender(config, queue, queue, format_map, packet_pool, byte_buffer_pool,
                  allocator);

    CHECK(sender.valid());

    FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame * NumCh);
    }

    PacketReader packet_reader(queue, rtp_parser, pcm_decoder, packet_pool, PayloadType,
                               config.source_port.address);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

TEST(sender, frame_size_large) {
    enum {
        SamplesPerLargeFrame = SamplesPerPacket * 4,
        PacketsPerLargeFrame = SamplesPerLargeFrame / SamplesPerPacket,
        ManyLargeFrames = 20
    };

    packet::ConcurrentQueue queue(0, false);

    Sender sender(config, queue, queue, format_map, packet_pool, byte_buffer_pool,
                  allocator);

    CHECK(sender.valid());

    FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame * NumCh);
    }

    PacketReader packet_reader(queue, rtp_parser, pcm_decoder, packet_pool, PayloadType,
                               config.source_port.address);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

} // namespace pipeline
} // namespace roc
