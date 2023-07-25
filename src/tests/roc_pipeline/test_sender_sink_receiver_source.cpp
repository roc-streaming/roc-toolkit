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
#include "test_helpers/packet_sender.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_allocator.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

namespace {

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

    ManyFrames = Latency / SamplesPerFrame * 10
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
    FlagLDPC = (1 << 5)
};

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);
core::BufferFactory<uint8_t> byte_buffer_factory(allocator, MaxBufSize, true);
packet::PacketFactory packet_factory(allocator, true);
rtp::FormatMap format_map(allocator, true);

SenderConfig
make_sender_config(int flags, size_t frame_channels, size_t packet_channels) {
    SenderConfig config;

    config.input_sample_spec.set_sample_rate(SampleRate);
    config.input_sample_spec.channel_set().clear_channels();
    config.input_sample_spec.channel_set().set_layout(
        frame_channels == 1 ? audio::ChannelLayout_Mono : audio::ChannelLayout_Surround);
    config.input_sample_spec.channel_set().set_channel_range(0, frame_channels - 1, true);

    switch (packet_channels) {
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
    config.internal_frame_length = MaxBufSize * core::Second
        / core::nanoseconds_t(SampleRate * std::max(frame_channels, packet_channels));

    if (flags & FlagReedSolomon) {
        config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;
    } else if (flags & FlagLDPC) {
        config.fec_encoder.scheme = packet::FEC_LDPC_Staircase;
    }

    config.fec_writer.n_source_packets = SourcePackets;
    config.fec_writer.n_repair_packets = RepairPackets;

    config.enable_interleaving = (flags & FlagInterleaving);
    config.enable_timing = false;
    config.enable_poisoning = true;
    config.enable_profiling = true;

    return config;
}

ReceiverConfig make_receiver_config(size_t frame_channels, size_t packet_channels) {
    ReceiverConfig config;

    config.common.output_sample_spec.set_sample_rate(SampleRate);
    config.common.output_sample_spec.channel_set().clear_channels();
    config.common.output_sample_spec.channel_set().set_layout(
        frame_channels == 1 ? audio::ChannelLayout_Mono : audio::ChannelLayout_Surround);
    config.common.output_sample_spec.channel_set().set_channel_range(
        0, frame_channels - 1, true);

    config.common.internal_frame_length = MaxBufSize * core::Second
        / core::nanoseconds_t(SampleRate * std::max(frame_channels, packet_channels));

    config.common.enable_resampling = false;
    config.common.enable_timing = false;
    config.common.enable_poisoning = true;

    config.default_session.target_latency = Latency * core::Second / SampleRate;
    config.default_session.watchdog.no_playback_timeout =
        Timeout * core::Second / SampleRate;

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

    while (packet::PacketPtr pp = reader.read()) {
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

        writer.write(pp);
    }
}

void send_receive(int flags,
                  size_t num_sessions,
                  size_t frame_channels,
                  size_t packet_channels) {
    packet::Queue queue;

    address::Protocol source_proto = select_source_proto(flags);
    address::Protocol repair_proto = select_repair_proto(flags);

    address::SocketAddr receiver_source_addr = test::new_address(11);
    address::SocketAddr receiver_repair_addr = test::new_address(22);

    SenderConfig sender_config =
        make_sender_config(flags, frame_channels, packet_channels);

    SenderSink sender(sender_config, format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);

    CHECK(sender.is_valid());

    SenderSlot* sender_slot = sender.create_slot();
    CHECK(sender_slot);

    SenderEndpoint* sender_source_endpoint = NULL;
    SenderEndpoint* sender_repair_endpoint = NULL;

    sender_source_endpoint =
        sender_slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(sender_source_endpoint);

    sender_source_endpoint->set_destination_writer(queue);
    sender_source_endpoint->set_destination_address(receiver_source_addr);

    if (repair_proto != address::Proto_None) {
        sender_repair_endpoint =
            sender_slot->create_endpoint(address::Iface_AudioRepair, repair_proto);
        CHECK(sender_repair_endpoint);

        sender_repair_endpoint->set_destination_writer(queue);
        sender_repair_endpoint->set_destination_address(receiver_repair_addr);
    }

    ReceiverConfig receiver_config =
        make_receiver_config(frame_channels, packet_channels);

    ReceiverSource receiver(receiver_config, format_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, allocator);

    CHECK(receiver.is_valid());

    ReceiverSlot* receiver_slot = receiver.create_slot();
    CHECK(receiver_slot);

    ReceiverEndpoint* receiver_source_endpoint = NULL;
    ReceiverEndpoint* receiver_repair_endpoint = NULL;

    packet::IWriter* receiver_source_endpoint_writer = NULL;
    packet::IWriter* receiver_repair_endpoint_writer = NULL;

    receiver_source_endpoint =
        receiver_slot->create_endpoint(address::Iface_AudioSource, source_proto);
    CHECK(receiver_source_endpoint);
    receiver_source_endpoint_writer = &receiver_source_endpoint->writer();

    if (repair_proto != address::Proto_None) {
        receiver_repair_endpoint =
            receiver_slot->create_endpoint(address::Iface_AudioRepair, repair_proto);
        CHECK(receiver_repair_endpoint);
        receiver_repair_endpoint_writer = &receiver_repair_endpoint->writer();
    }

    test::FrameWriter frame_writer(sender, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, sender_config.input_sample_spec);
    }

    test::PacketSender packet_sender(packet_factory, receiver_source_endpoint_writer,
                                     receiver_repair_endpoint_writer);

    filter_packets(flags, queue, packet_sender);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    packet_sender.deliver(Latency / SamplesPerPacket);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame, num_sessions,
                                      receiver_config.common.output_sample_spec);

            UNSIGNED_LONGS_EQUAL(num_sessions, receiver.num_sessions());
        }

        packet_sender.deliver(1);
    }
}

} // namespace

TEST_GROUP(sender_sink_receiver_source) {};

TEST(sender_sink_receiver_source, bare) {
    enum { NumSess = 1, NumCh = 2 };

    send_receive(FlagNone, NumSess, NumCh, NumCh);
}

TEST(sender_sink_receiver_source, interleaving) {
    enum { NumSess = 1, NumCh = 2 };

    send_receive(FlagInterleaving, NumSess, NumCh, NumCh);
}

TEST(sender_sink_receiver_source, fec_rs) {
    enum { NumSess = 1, NumCh = 2 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon, NumSess, NumCh, NumCh);
    }
}

TEST(sender_sink_receiver_source, fec_ldpc) {
    enum { NumSess = 1, NumCh = 2 };

    if (is_fec_supported(FlagLDPC)) {
        send_receive(FlagLDPC, NumSess, NumCh, NumCh);
    }
}

TEST(sender_sink_receiver_source, fec_interleaving) {
    enum { NumSess = 1, NumCh = 2 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagInterleaving, NumSess, NumCh, NumCh);
    }
}

TEST(sender_sink_receiver_source, fec_loss) {
    enum { NumSess = 1, NumCh = 2 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagLosses, NumSess, NumCh, NumCh);
    }
}

TEST(sender_sink_receiver_source, fec_drop_source) {
    enum { NumSess = 0, NumCh = 2 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagDropSource, NumSess, NumCh, NumCh);
    }
}

TEST(sender_sink_receiver_source, fec_drop_repair) {
    enum { NumSess = 1, NumCh = 2 };

    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagDropRepair, NumSess, NumCh, NumCh);
    }
}

TEST(sender_sink_receiver_source, channels_stereo_to_mono) {
    enum { NumSess = 1, FrameCh = 2, PacketCh = 1 };

    send_receive(FlagNone, NumSess, FrameCh, PacketCh);
}

TEST(sender_sink_receiver_source, channels_mono_to_stereo) {
    enum { NumSess = 1, FrameCh = 1, PacketCh = 2 };

    send_receive(FlagNone, NumSess, FrameCh, PacketCh);
}

} // namespace pipeline
} // namespace roc
