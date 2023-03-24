/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_writer.h"
#include "test_helpers/packet_reader.h"

#include "roc_audio/pcm_decoder.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/time.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"

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

const audio::SampleSpec SampleSpecs = audio::SampleSpec(SampleRate, ChMask);

const core::nanoseconds_t MaxBufDuration = MaxBufSize * core::Second
    / core::nanoseconds_t(SampleSpecs.sample_rate() * SampleSpecs.num_channels());

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);
core::BufferFactory<uint8_t> byte_buffer_factory(allocator, MaxBufSize, true);
packet::PacketFactory packet_factory(allocator, true);

rtp::FormatMap format_map;
rtp::Parser rtp_parser(format_map, NULL);

} // namespace

TEST_GROUP(sender_sink) {
    SenderConfig config;

    address::Protocol source_proto;
    address::SocketAddr dst_addr;

    void setup() {
        config.input_sample_spec = audio::SampleSpec(SampleRate, ChMask);
        config.packet_length = SamplesPerPacket * core::Second / SampleRate;
        config.internal_frame_length = MaxBufDuration;

        config.interleaving = false;
        config.timing = false;
        config.poisoning = true;
        config.profiling = true;

        source_proto = address::Proto_RTP;
        dst_addr = test::new_address(123);
    }
};

TEST(sender_sink, write) {
    packet::Queue queue;

    SenderSink sender(config, format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * NumCh);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, SampleSpecs);
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

    SenderSink sender(config, format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame * NumCh);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType, dst_addr);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, SampleSpecs);
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

    SenderSink sender(config, format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame * NumCh);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType, dst_addr);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, SampleSpecs);
    }

    CHECK(!queue.read());
}

} // namespace pipeline
} // namespace roc
