/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_reader.h"
#include "test_helpers/frame_writer.h"
#include "test_helpers/packet_proxy.h"

#include "roc_address/interface.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/time.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/encoding_map.h"

// This file contains integration tests that combine SenderSink and ReceiverSource.
//
// SenderSink consumes audio frames and produces network packets. ReceiverSource
// consumes network packets and produces audio frames.
//
// Each test in this file prepares a sequence of input frames, passes it to
// SenderSink, transfers packets produced by SenderSink to ReceiverSource, and
// checks what sequence of output frames ReceiverSource produced in response.
//
// Normally SenderSink and ReceiverSource are not connected directly. We simulate
// delivering packets over network by re-creating packets for receiver with the
// same buffer but with stripped meta-information.
//
// The tests use three helper classes:
//  - test::FrameWriter - to produce frames
//  - test::PacketProxy - to simulate delivery of packets from sender to receiver
//  - test::FrameReader - to retrieve and validate frames
//
// test::FrameWriter simulates sender sound card that produces frames, and
// test::FrameReader simulates receiver sound card that consumes frames.

namespace roc {
namespace pipeline {

namespace {

const audio::ChannelMask Chans_Mono = audio::ChanMask_Surround_Mono;
const audio::ChannelMask Chans_Stereo = audio::ChanMask_Surround_Stereo;

const rtp::PayloadType PayloadType_Ch1 = rtp::PayloadType_L16_Mono;
const rtp::PayloadType PayloadType_Ch2 = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 500,

    SampleRate = 44100,

    SamplesPerFrame = 10,
    SamplesPerPacket = 40,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    SourcePackets = 20,
    RepairPackets = 10,

    Latency = SamplesPerPacket * SourcePackets,
    Timeout = Latency * 20,

    ManyFrames = Latency / SamplesPerFrame * 10,
};

enum {
    // default flags
    FlagNone = 0,

    // drop all source packets on receiver
    FlagDropSource = (1 << 0),

    // drop all repair packets on receiver
    FlagDropRepair = (1 << 1),

    // enable packet losses on sender
    FlagLosses = (1 << 2),

    // enable packet interleaving on sender
    FlagInterleaving = (1 << 3),

    // enable Reed-Solomon FEC scheme on sender
    FlagReedSolomon = (1 << 4),

    // enable LDPC-Staircase FEC scheme on sender
    FlagLDPC = (1 << 5),

    // enable RTCP traffic
    FlagRTCP = (1 << 6),

    // enable capture timestamps
    FlagCTS = (1 << 7)
};

core::HeapArena arena;
core::BufferFactory<audio::sample_t> sample_buffer_factory(arena, MaxBufSize);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, MaxBufSize);
packet::PacketFactory packet_factory(arena);
rtp::EncodingMap encoding_map(arena);

SenderConfig make_sender_config(int flags,
                                audio::ChannelMask frame_channels,
                                audio::ChannelMask packet_channels) {
    SenderConfig config;

    config.input_sample_spec.set_sample_rate(SampleRate);
    config.input_sample_spec.set_sample_format(audio::SampleFormat_Pcm);
    config.input_sample_spec.set_pcm_format(audio::Sample_RawFormat);
    config.input_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
    config.input_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
    config.input_sample_spec.channel_set().set_channel_mask(frame_channels);

    switch (packet_channels) {
    case Chans_Mono:
        config.payload_type = PayloadType_Ch1;
        break;
    case Chans_Stereo:
        config.payload_type = PayloadType_Ch2;
        break;
    default:
        FAIL("unsupported packet_sample_spec");
    }

    config.packet_length = SamplesPerPacket * core::Second / SampleRate;

    if (flags & FlagReedSolomon) {
        config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;
    } else if (flags & FlagLDPC) {
        config.fec_encoder.scheme = packet::FEC_LDPC_Staircase;
    }

    config.fec_writer.n_source_packets = SourcePackets;
    config.fec_writer.n_repair_packets = RepairPackets;

    config.enable_interleaving = (flags & FlagInterleaving);
    config.enable_timing = false;
    config.enable_profiling = true;

    config.rtcp_config.inactivity_timeout = Timeout * core::Second / SampleRate;

    return config;
}

ReceiverConfig make_receiver_config(audio::ChannelMask frame_channels,
                                    audio::ChannelMask packet_channels) {
    ReceiverConfig config;

    config.common.output_sample_spec.set_sample_rate(SampleRate);
    config.common.output_sample_spec.set_sample_format(audio::SampleFormat_Pcm);
    config.common.output_sample_spec.set_pcm_format(audio::Sample_RawFormat);
    config.common.output_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
    config.common.output_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
    config.common.output_sample_spec.channel_set().set_channel_mask(frame_channels);

    config.common.enable_timing = false;

    config.common.rtcp_config.inactivity_timeout = Timeout * core::Second / SampleRate;

    config.default_session.latency_monitor.fe_enable = false;
    config.default_session.target_latency = Latency * core::Second / SampleRate;
    config.default_session.watchdog.no_playback_timeout =
        Timeout * core::Second / SampleRate;

    (void)packet_channels;

    return config;
}

address::Protocol select_source_proto(int flags) {
    if (flags & FlagReedSolomon) {
        return address::Proto_RTP_RS8M_Source;
    }
    if (flags & FlagLDPC) {
        return address::Proto_RTP_LDPC_Source;
    }
    return address::Proto_RTP;
}

address::Protocol select_repair_proto(int flags) {
    if (flags & FlagReedSolomon) {
        return address::Proto_RS8M_Repair;
    }
    if (flags & FlagLDPC) {
        return address::Proto_LDPC_Repair;
    }
    return address::Proto_None;
}

address::Protocol select_control_proto(int flags) {
    if (flags & FlagRTCP) {
        return address::Proto_RTCP;
    }
    return address::Proto_None;
}

bool is_fec_supported(int flags) {
    if (flags & FlagReedSolomon) {
        return fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8);
    }
    if (flags & FlagLDPC) {
        return fec::CodecMap::instance().is_supported(packet::FEC_LDPC_Staircase);
    }
    return true;
}

void filter_packets(int flags, packet::IReader& reader, packet::IWriter& writer) {
    size_t counter = 0;

    packet::PacketPtr pp;
    while (reader.read(pp) == status::StatusOK) {
        if ((flags & FlagLosses) && counter++ % (SourcePackets + RepairPackets) == 1) {
            continue;
        }

        if (pp->flags() & packet::Packet::FlagRepair) {
            if (flags & FlagDropRepair) {
                continue;
            }
        } else {
            if (flags & FlagDropSource) {
                continue;
            }
        }

        UNSIGNED_LONGS_EQUAL(status::StatusOK, writer.write(pp));
    }
}

void send_receive(int flags,
                  size_t num_sessions,
                  audio::ChannelMask frame_channels,
                  audio::ChannelMask packet_channels) {
    packet::Queue queue;

    address::Protocol source_proto = select_source_proto(flags);
    address::Protocol repair_proto = select_repair_proto(flags);
    address::Protocol control_proto = select_control_proto(flags);

    address::SocketAddr receiver_source_addr = test::new_address(11);
    address::SocketAddr receiver_repair_addr = test::new_address(22);
    address::SocketAddr receiver_control_addr = test::new_address(33);

    address::SocketAddr sender_addr = test::new_address(44);

    SenderConfig sender_config =
        make_sender_config(flags, frame_channels, packet_channels);

    SenderSink sender(sender_config, encoding_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderSlot* sender_slot = sender.create_slot();
    CHECK(sender_slot);

    SenderEndpoint* sender_source_endpoint = NULL;
    SenderEndpoint* sender_repair_endpoint = NULL;
    SenderEndpoint* sender_control_endpoint = NULL;

    sender_source_endpoint = sender_slot->add_endpoint(
        address::Iface_AudioSource, source_proto, receiver_source_addr, queue);
    CHECK(sender_source_endpoint);

    if (repair_proto != address::Proto_None) {
        sender_repair_endpoint = sender_slot->add_endpoint(
            address::Iface_AudioRepair, repair_proto, receiver_repair_addr, queue);
        CHECK(sender_repair_endpoint);
    }

    if (control_proto != address::Proto_None) {
        sender_control_endpoint = sender_slot->add_endpoint(
            address::Iface_AudioControl, control_proto, receiver_control_addr, queue);
        CHECK(sender_control_endpoint);
    }

    ReceiverConfig receiver_config =
        make_receiver_config(frame_channels, packet_channels);

    ReceiverSource receiver(receiver_config, encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* receiver_slot = receiver.create_slot();
    CHECK(receiver_slot);

    ReceiverEndpoint* receiver_source_endpoint = NULL;
    ReceiverEndpoint* receiver_repair_endpoint = NULL;
    ReceiverEndpoint* receiver_control_endpoint = NULL;

    packet::IWriter* receiver_source_endpoint_writer = NULL;
    packet::IWriter* receiver_repair_endpoint_writer = NULL;
    packet::IWriter* receiver_control_endpoint_writer = NULL;

    receiver_source_endpoint =
        receiver_slot->add_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(receiver_source_endpoint);
    receiver_source_endpoint_writer = &receiver_source_endpoint->writer();

    if (repair_proto != address::Proto_None) {
        receiver_repair_endpoint =
            receiver_slot->add_endpoint(address::Iface_AudioRepair, repair_proto);
        CHECK(receiver_repair_endpoint);
        receiver_repair_endpoint_writer = &receiver_repair_endpoint->writer();
    }

    if (control_proto != address::Proto_None) {
        receiver_control_endpoint =
            receiver_slot->add_endpoint(address::Iface_AudioControl, control_proto);
        CHECK(receiver_control_endpoint);
        receiver_control_endpoint_writer = &receiver_control_endpoint->writer();
    }

    core::nanoseconds_t send_base_cts = -1;
    if (flags & FlagCTS) {
        send_base_cts = 1000000000000000;
    }

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, sender_config.input_sample_spec,
                                   send_base_cts);
        sender.refresh(frame_writer.last_capture_ts());
    }

    test::PacketProxy packet_proxy(
        packet_factory, sender_addr, receiver_source_endpoint_writer,
        receiver_repair_endpoint_writer, receiver_control_endpoint_writer);

    filter_packets(flags, queue, packet_proxy);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    packet_proxy.deliver(Latency / SamplesPerPacket);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            core::nanoseconds_t recv_base_cts = -1;
            if (flags & FlagCTS) {
                recv_base_cts = send_base_cts;
            }

            receiver.refresh(frame_reader.refresh_ts(recv_base_cts));

            frame_reader.read_samples(SamplesPerFrame, num_sessions,
                                      receiver_config.common.output_sample_spec,
                                      recv_base_cts);

            UNSIGNED_LONGS_EQUAL(num_sessions, receiver.num_sessions());
        }

        packet_proxy.deliver(1);
    }

    if ((flags & FlagDropSource) == 0) {
        CHECK(packet_proxy.n_source() > 0);
    } else {
        CHECK(packet_proxy.n_source() == 0);
    }

    if ((flags & FlagDropRepair) == 0 && (flags & (FlagReedSolomon | FlagLDPC)) != 0) {
        CHECK(packet_proxy.n_repair() > 0);
    } else {
        CHECK(packet_proxy.n_repair() == 0);
    }

    if ((flags & FlagRTCP) != 0) {
        CHECK(packet_proxy.n_control() > 0);
    } else {
        CHECK(packet_proxy.n_control() == 0);
    }
}

} // namespace

TEST_GROUP(loopback_sink_2_source) {};

TEST(loopback_sink_2_source, bare_rtp) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    send_receive(FlagNone, NumSess, Chans, Chans);
}

TEST(loopback_sink_2_source, interleaving) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    send_receive(FlagInterleaving, NumSess, Chans, Chans);
}

TEST(loopback_sink_2_source, fec_rs) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon, NumSess, Chans, Chans);
    }
}

TEST(loopback_sink_2_source, fec_ldpc) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    if (is_fec_supported(FlagLDPC)) {
        send_receive(FlagLDPC, NumSess, Chans, Chans);
    }
}

TEST(loopback_sink_2_source, fec_interleaving) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagInterleaving, NumSess, Chans, Chans);
    }
}

TEST(loopback_sink_2_source, fec_loss) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagLosses, NumSess, Chans, Chans);
    }
}

TEST(loopback_sink_2_source, fec_drop_source) {
    enum { Chans = Chans_Stereo, NumSess = 0 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagDropSource, NumSess, Chans, Chans);
    }
}

TEST(loopback_sink_2_source, fec_drop_repair) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagDropRepair, NumSess, Chans, Chans);
    }
}

TEST(loopback_sink_2_source, channel_mapping_stereo_to_mono) {
    enum { FrameChans = Chans_Stereo, PacketChans = Chans_Mono, NumSess = 1 };

    send_receive(FlagNone, NumSess, FrameChans, PacketChans);
}

TEST(loopback_sink_2_source, channel_mapping_mono_to_stereo) {
    enum { FrameChans = Chans_Mono, PacketChans = Chans_Stereo, NumSess = 1 };

    send_receive(FlagNone, NumSess, FrameChans, PacketChans);
}

TEST(loopback_sink_2_source, timestamp_mapping) {
    enum { Chans = Chans_Stereo, NumSess = 1 };

    send_receive(FlagRTCP | FlagCTS, NumSess, Chans, Chans);
}

TEST(loopback_sink_2_source, timestamp_mapping_remixing) {
    enum { FrameChans = Chans_Mono, PacketChans = Chans_Stereo, NumSess = 1 };

    send_receive(FlagRTCP | FlagCTS, NumSess, FrameChans, PacketChans);
}

} // namespace pipeline
} // namespace roc
