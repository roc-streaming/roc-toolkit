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
#include "roc_rtp/headers.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace pipeline {

namespace {

const rtp::PayloadType PayloadType_Ch1 = rtp::PayloadType_L16_Mono;
const rtp::PayloadType PayloadType_Ch2 = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 1000,

    SampleRate = 44100,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    ManyFrames = FramesPerPacket * 20
};

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);
core::BufferFactory<uint8_t> byte_buffer_factory(allocator, MaxBufSize, true);
packet::PacketFactory packet_factory(allocator, true);

rtp::FormatMap format_map(allocator, true);
rtp::Parser rtp_parser(format_map, NULL);

} // namespace

TEST_GROUP(sender_sink) {
    audio::SampleSpec input_sample_spec;
    audio::SampleSpec packet_sample_spec;

    address::Protocol source_proto;
    address::SocketAddr dst_addr;

    SenderConfig make_config() {
        SenderConfig config;

        config.input_sample_spec = input_sample_spec;

        switch (packet_sample_spec.num_channels()) {
        case 1:
            config.payload_type = PayloadType_Ch1;
            break;
        case 2:
            config.payload_type = PayloadType_Ch2;
            break;
        default:
            FAIL("unsupported packet_sample_spec");
        }

        config.packet_length = SamplesPerPacket * core::Second / SampleRate;

        config.enable_interleaving = false;
        config.enable_timing = false;
        config.enable_poisoning = true;
        config.enable_profiling = true;

        return config;
    }

    void init(size_t input_channels, size_t packet_channels) {
        input_sample_spec.set_sample_rate(SampleRate);
        input_sample_spec.channel_set().set_layout(input_channels == 1
                                                       ? audio::ChannelLayout_Mono
                                                       : audio::ChannelLayout_Surround);
        input_sample_spec.channel_set().set_channel_range(0, input_channels - 1, true);

        packet_sample_spec.set_sample_rate(SampleRate);
        packet_sample_spec.channel_set().set_layout(packet_channels == 1
                                                        ? audio::ChannelLayout_Mono
                                                        : audio::ChannelLayout_Surround);
        packet_sample_spec.channel_set().set_channel_range(0, packet_channels - 1, true);

        source_proto = address::Proto_RTP;
        dst_addr = test::new_address(123);
    }
};

TEST(sender_sink, write) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, frame_size_small) {
    enum {
        NumCh = 2,
        SamplesPerSmallFrame = SamplesPerFrame / 2,
        SmallFramesPerPacket = SamplesPerPacket / SamplesPerSmallFrame,
        ManySmallFrames = SmallFramesPerPacket * 20
    };

    init(NumCh, NumCh);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame, input_sample_spec);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, frame_size_large) {
    enum {
        NumCh = 2,
        SamplesPerLargeFrame = SamplesPerPacket * 4,
        PacketsPerLargeFrame = SamplesPerLargeFrame / SamplesPerPacket,
        ManyLargeFrames = 20
    };

    init(NumCh, NumCh);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame, input_sample_spec);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, channels_stereo_to_mono) {
    enum { InputCh = 2, PacketCh = 1 };

    init(InputCh, PacketCh);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType_Ch1, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, channels_mono_to_stereo) {
    enum { InputCh = 1, PacketCh = 2 };

    init(InputCh, PacketCh);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    source_endpoint->set_destination_writer(queue);
    source_endpoint->set_destination_address(dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_factory, PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    CHECK(!queue.read());
}

} // namespace pipeline
} // namespace roc
