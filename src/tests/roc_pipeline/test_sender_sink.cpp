/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/control_writer.h"
#include "test_helpers/frame_writer.h"
#include "test_helpers/packet_reader.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/time.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/encoding_map.h"

// This file contains tests for SenderSink. SenderSink can be seen as a big
// composite processor (consisting of chanined smaller processors) that transforms
// audio frames into network packets. Typically, sound card thread writes frames
// to SenderSink, and it in turn writes packets to network thread.
//
// Each test in this file prepares a sequence of input frames and checks what sequence
// of output packets sender produces in response. Each test checks one aspect of
// pipeline behavior, e.g. splitting frames into packets, transcoding, etc.
//
// The tests mostly use two helper classes:
//  - test::FrameWriter - to produce frames
//  - test::PacketReader - to retrieve and validate packets
//
// test::FrameWriter simulates local sound card that produces frames, and
// test::PacketReader simulates remote receiver that consumes packets.

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

rtp::EncodingMap encoding_map(arena);

SenderSlot* create_slot(SenderSink& sink) {
    SenderSlot* slot = sink.create_slot();
    CHECK(slot);
    return slot;
}

void create_transport_endpoint(SenderSlot* slot,
                               address::Interface iface,
                               address::Protocol proto,
                               const address::SocketAddr& outbound_address,
                               packet::IWriter& outbound_writer) {
    CHECK(slot);
    SenderEndpoint* endpoint =
        slot->add_endpoint(iface, proto, outbound_address, outbound_writer);
    CHECK(endpoint);
    CHECK(!endpoint->inbound_writer());
}

packet::IWriter* create_control_endpoint(SenderSlot* slot,
                                         address::Interface iface,
                                         address::Protocol proto,
                                         const address::SocketAddr& outbound_address,
                                         packet::IWriter& outbound_writer) {
    CHECK(slot);
    SenderEndpoint* endpoint =
        slot->add_endpoint(iface, proto, outbound_address, outbound_writer);
    CHECK(endpoint);
    CHECK(endpoint->inbound_writer());
    return endpoint->inbound_writer();
}

} // namespace

TEST_GROUP(sender_sink) {
    audio::SampleSpec input_sample_spec;
    audio::SampleSpec packet_sample_spec;

    address::Protocol proto;

    address::SocketAddr src_addr;
    address::SocketAddr dst_addr;

    packet::stream_source_t src_ssrc;
    packet::stream_source_t dst_ssrc;

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

        config.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        config.latency.tuner_profile = audio::LatencyTunerProfile_Intact;

        return config;
    }

    void init(int input_sample_rate, audio::ChannelMask input_channels,
              int packet_sample_rate, audio::ChannelMask packet_channels) {
        input_sample_spec.set_sample_rate((size_t)input_sample_rate);
        input_sample_spec.set_sample_format(audio::SampleFormat_Pcm);
        input_sample_spec.set_pcm_format(audio::Sample_RawFormat);
        input_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        input_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        input_sample_spec.channel_set().set_channel_mask(input_channels);

        packet_sample_spec.set_sample_rate((size_t)packet_sample_rate);
        packet_sample_spec.set_sample_format(audio::SampleFormat_Pcm);
        packet_sample_spec.set_pcm_format(audio::PcmFormat_SInt16_Be);
        packet_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        packet_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        packet_sample_spec.channel_set().set_channel_mask(packet_channels);

        proto = address::Proto_RTP;

        src_addr = test::new_address(111);
        dst_addr = test::new_address(222);

        src_ssrc = 0;
        dst_ssrc = 0;
    }
};

TEST(sender_sink, write) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames smaller than packets.
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

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames larger than packets.
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

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames written to sender are stereo, packets are mono.
TEST(sender_sink, channel_mapping_stereo_to_mono) {
    enum { Rate = SampleRate, InputChans = Chans_Stereo, PacketChans = Chans_Mono };

    init(Rate, InputChans, Rate, PacketChans);

    packet::Queue queue;

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch1);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames written to sender are mono, packets are stereo.
TEST(sender_sink, channel_mapping_mono_to_stereo) {
    enum { Rate = SampleRate, InputChans = Chans_Mono, PacketChans = Chans_Stereo };

    init(Rate, InputChans, Rate, PacketChans);

    packet::Queue queue;

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Different sample rate of frames and packets.
TEST(sender_sink, sample_rate_mapping) {
    enum { InputRate = 48000, PacketRate = 44100, Chans = Chans_Stereo };

    init(InputRate, Chans, PacketRate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * InputRate / PacketRate
                                       / input_sample_spec.num_channels()
                                       * input_sample_spec.num_channels(),
                                   input_sample_spec);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket - 5; np++) {
        packet_reader.read_nonzero_packet(SamplesPerPacket, packet_sample_spec);
    }
}

// Check how sender sets CTS of packets based on CTS of frames
// written to it.
TEST(sender_sink, timestamp_mapping) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);
    }

    packet_reader.read_eof();
}

// Same as above, but there is also channel conversion and sample rate conversion.
TEST(sender_sink, timestamp_mapping_remixing) {
    enum {
        InputRate = 48000,
        PacketRate = 44100,
        InputChans = Chans_Stereo,
        PacketChans = Chans_Mono
    };

    init(InputRate, InputChans, PacketRate, PacketChans);

    packet::Queue queue;

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * InputRate / PacketRate
                                       / input_sample_spec.num_channels()
                                       * input_sample_spec.num_channels(),
                                   input_sample_spec, unix_base);
        sender.refresh(frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch1);

    core::nanoseconds_t cts = 0;
    for (size_t np = 0; np < ManyFrames / FramesPerPacket - 5; np++) {
        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(pp));
        CHECK(pp);

        if (np == 0) {
            cts = pp->rtp()->capture_timestamp;
            CHECK(cts >= unix_base);
            CHECK(cts < unix_base + core::Millisecond);
        } else {
            test::expect_capture_timestamp(cts, pp->rtp()->capture_timestamp,
                                           packet_sample_spec,
                                           test::TimestampEpsilonSmpls);
        }
        cts += packet_sample_spec.samples_per_chan_2_ns(pp->rtp()->duration);
    }
}

// Check sender metrics for multiple remote participants (receiver).
IGNORE_TEST(sender_sink, metrics_participants) {
    // TODO(gh-674): add test for multiple receivers
}

// Check how sender returns metrics if provided buffer for metrics
// is smaller than needed.
IGNORE_TEST(sender_sink, metrics_truncation) {
    // TODO(gh-674): add test for multiple receivers
}

// Check how sender fills metrics from feedback reports of remote receiver.
TEST(sender_sink, metrics_feedback) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, MaxParties = 10 };

    init(Rate, Chans, Rate, Chans);

    packet::Queue queue;

    SenderSink sender(make_config(), encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* slot = create_slot(sender);
    CHECK(slot);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr, queue);

    packet::Queue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr, control_outbound_queue);
    CHECK(control_endpoint);

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory, dst_addr,
                                     PayloadType_Ch2);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
        sender.refresh(frame_writer.refresh_ts());
    }

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);
    }

    CHECK(control_outbound_queue.size() > 0);

    {
        SenderSlotMetrics slot_metrics;
        SenderParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = MaxParties;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        CHECK(slot_metrics.source_id != 0);

        src_ssrc = slot_metrics.source_id;
        dst_ssrc = src_ssrc + 99999;

        UNSIGNED_LONGS_EQUAL(0, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(0, party_metrics_size);
    }

    test::ControlWriter control_writer(*control_endpoint, packet_factory,
                                       byte_buffer_factory, dst_addr, src_addr);

    control_writer.set_local_source(dst_ssrc);
    control_writer.set_remote_source(src_ssrc);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        const unsigned seed = (unsigned)np + 1;

        packet::LinkMetrics link_metrics;
        link_metrics.ext_first_seqnum = seed * 100;
        link_metrics.ext_last_seqnum = seed * 200;
        link_metrics.total_packets = (seed * 200) - (seed * 100) + 1;
        link_metrics.lost_packets = (int)seed * 40;
        link_metrics.jitter = (int)seed * core::Millisecond * 50;

        audio::LatencyMetrics latency_metrics;
        latency_metrics.niq_latency = (int)seed * core::Millisecond * 50;
        latency_metrics.niq_stalling = (int)seed * core::Millisecond * 60;
        latency_metrics.e2e_latency = (int)seed * core::Millisecond * 70;

        control_writer.set_link_metrics(link_metrics);
        control_writer.set_latency_metrics(latency_metrics);

        control_writer.write_receiver_report(packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
            sender.refresh(frame_writer.refresh_ts());
        }
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);

        {
            SenderSlotMetrics slot_metrics;
            SenderParticipantMetrics party_metrics[MaxParties];
            size_t party_metrics_size = MaxParties;

            slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

            UNSIGNED_LONGS_EQUAL(src_ssrc, slot_metrics.source_id);
            UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

            UNSIGNED_LONGS_EQUAL(link_metrics.ext_first_seqnum,
                                 party_metrics[0].link.ext_first_seqnum);
            UNSIGNED_LONGS_EQUAL(link_metrics.ext_last_seqnum,
                                 party_metrics[0].link.ext_last_seqnum);
            UNSIGNED_LONGS_EQUAL(link_metrics.total_packets,
                                 party_metrics[0].link.total_packets);
            UNSIGNED_LONGS_EQUAL(link_metrics.lost_packets,
                                 party_metrics[0].link.lost_packets);
            DOUBLES_EQUAL((double)link_metrics.jitter,
                          (double)party_metrics[0].link.jitter, core::Nanosecond);

            DOUBLES_EQUAL((double)latency_metrics.niq_latency,
                          (double)party_metrics[0].latency.niq_latency,
                          core::Microsecond * 16);
            DOUBLES_EQUAL((double)latency_metrics.niq_stalling,
                          (double)party_metrics[0].latency.niq_stalling,
                          core::Microsecond * 16);
            DOUBLES_EQUAL((double)latency_metrics.e2e_latency,
                          (double)party_metrics[0].latency.e2e_latency, core::Nanosecond);
        }
    }
}

} // namespace pipeline
} // namespace roc
