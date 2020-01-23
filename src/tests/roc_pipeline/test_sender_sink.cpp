/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_funcs.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"

#include "test_frame_writer.h"
#include "test_packet_reader.h"

namespace roc {
namespace pipeline {

namespace {

rtp::PayloadType PayloadType = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 1000,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    ManyFrames = FramesPerPacket * 20
};

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, true);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

rtp::FormatMap format_map;
rtp::Parser rtp_parser(format_map, NULL);

} // namespace

TEST_GROUP(sender_sink) {
    SenderConfig config;

    PortConfig source_port;
    PortConfig repair_port;

    void setup() {
        source_port.address = new_address(1);
        source_port.protocol = address::EndProto_RTP;

        config.input_channels = ChMask;
        config.packet_length = SamplesPerPacket * core::Second / SampleRate;
        config.internal_frame_size = MaxBufSize;

        config.interleaving = false;
        config.timing = false;
        config.poisoning = true;
    }
};

TEST(sender_sink, write) {
    packet::Queue queue;

    SenderSink sender(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    SenderSink::PortGroupID port_group = sender.add_port_group();
    CHECK(port_group != 0);

    SenderSink::PortID source_port_id =
        sender.add_port(port_group, address::EndType_AudioSource, source_port);
    CHECK(source_port_id != 0);
    sender.set_port_writer(source_port_id, queue);

    FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * NumCh);
    }

    PacketReader packet_reader(allocator, queue, rtp_parser, format_map, packet_pool,
                               PayloadType, source_port.address);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, frame_size_small) {
    enum {
        SamplesPerSmallFrame = SamplesPerFrame / 2,
        SmallFramesPerPacket = SamplesPerPacket / SamplesPerSmallFrame,
        ManySmallFrames = SmallFramesPerPacket * 20
    };

    packet::Queue queue;

    SenderSink sender(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    SenderSink::PortGroupID port_group = sender.add_port_group();
    CHECK(port_group != 0);

    SenderSink::PortID source_port_id =
        sender.add_port(port_group, address::EndType_AudioSource, source_port);
    CHECK(source_port_id != 0);
    sender.set_port_writer(source_port_id, queue);

    FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame * NumCh);
    }

    PacketReader packet_reader(allocator, queue, rtp_parser, format_map, packet_pool,
                               PayloadType, source_port.address);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, frame_size_large) {
    enum {
        SamplesPerLargeFrame = SamplesPerPacket * 4,
        PacketsPerLargeFrame = SamplesPerLargeFrame / SamplesPerPacket,
        ManyLargeFrames = 20
    };

    packet::Queue queue;

    SenderSink sender(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    SenderSink::PortGroupID port_group = sender.add_port_group();
    CHECK(port_group != 0);

    SenderSink::PortID source_port_id =
        sender.add_port(port_group, address::EndType_AudioSource, source_port);
    CHECK(source_port_id != 0);
    sender.set_port_writer(source_port_id, queue);

    FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame * NumCh);
    }

    PacketReader packet_reader(allocator, queue, rtp_parser, format_map, packet_pool,
                               PayloadType, source_port.address);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

} // namespace pipeline
} // namespace roc
