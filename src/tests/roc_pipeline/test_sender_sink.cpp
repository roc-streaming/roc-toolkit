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
#include "roc_core/heap_arena.h"
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

const audio::ChannelMask Chans_Mono = audio::ChanMask_Surround_Mono;
const audio::ChannelMask Chans_Stereo = audio::ChanMask_Surround_Stereo;

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

core::HeapArena arena;
core::BufferFactory<audio::sample_t> sample_buffer_factory(arena, MaxBufSize);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, MaxBufSize);
packet::PacketFactory packet_factory(arena);

rtp::FormatMap format_map(arena);
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

        config.packet_length =
            SamplesPerPacket * core::Second / (int)packet_sample_spec.sample_rate();

        config.enable_interleaving = false;
        config.enable_timing = false;
        config.enable_profiling = true;

        return config;
    }

    void init(int input_sample_rate, audio::ChannelMask input_channels,
              int packet_sample_rate, audio::ChannelMask packet_channels) {
        input_sample_spec.set_sample_rate((size_t)input_sample_rate);
        input_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        input_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        input_sample_spec.channel_set().set_channel_mask(input_channels);

        packet_sample_spec.set_sample_rate((size_t)packet_sample_rate);
        packet_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        packet_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        packet_sample_spec.channel_set().set_channel_mask(packet_channels);

        source_proto = address::Proto_RTP;
        dst_addr = test::new_address(123);
    }
};

TEST(sender_sink, write) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

TEST(sender_sink, frame_size_small) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        SamplesPerSmallFrame = SamplesPerFrame / 2,
        SmallFramesPerPacket = SamplesPerPacket / SamplesPerSmallFrame,
        ManySmallFrames = SmallFramesPerPacket * 20
    };

    init(Rate, Chans, Rate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

TEST(sender_sink, frame_size_large) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        SamplesPerLargeFrame = SamplesPerPacket * 4,
        PacketsPerLargeFrame = SamplesPerLargeFrame / SamplesPerPacket,
        ManyLargeFrames = 20
    };

    init(Rate, Chans, Rate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

TEST(sender_sink, channel_mapping_stereo_to_mono) {
    enum { Rate = SampleRate, InputChans = Chans_Stereo, PacketChans = Chans_Mono };

    init(Rate, InputChans, Rate, PacketChans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch1, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

TEST(sender_sink, channel_mapping_mono_to_stereo) {
    enum { Rate = SampleRate, InputChans = Chans_Mono, PacketChans = Chans_Stereo };

    init(Rate, InputChans, Rate, PacketChans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

TEST(sender_sink, sample_rate_mapping) {
    enum { InputRate = 48000, PacketRate = 44100, Chans = Chans_Stereo };

    init(InputRate, Chans, PacketRate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * InputRate / PacketRate
                                       / input_sample_spec.num_channels()
                                       * input_sample_spec.num_channels(),
                                   input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket - 5; np++) {
        packet_reader.read_nonzero_packet(SamplesPerPacket, packet_sample_spec);
    }
}

TEST(sender_sink, timestamp_mapping) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch2, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);
    }

    packet_reader.read_eof();
}

IGNORE_TEST(sender_sink, timestamp_mapping_remixing) {
    enum {
        InputRate = 48000,
        PacketRate = 44100,
        InputChans = Chans_Stereo,
        PacketChans = Chans_Mono
    };

    init(InputRate, InputChans, PacketRate, PacketChans);

    packet::Queue queue;

    SenderSink sender(make_config(), format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = sender.create_slot();
    CHECK(slot);

    SenderEndpoint* source_endpoint =
        slot->add_endpoint(address::Iface_AudioSource, source_proto, dst_addr, queue);
    CHECK(source_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * InputRate / PacketRate
                                       / input_sample_spec.num_channels()
                                       * input_sample_spec.num_channels(),
                                   input_sample_spec, unix_base);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, rtp_parser, format_map, packet_factory,
                                     PayloadType_Ch1, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket - 5; np++) {
        packet_reader.read_nonzero_packet(SamplesPerPacket, packet_sample_spec,
                                          unix_base);
    }
}

} // namespace pipeline
} // namespace roc
