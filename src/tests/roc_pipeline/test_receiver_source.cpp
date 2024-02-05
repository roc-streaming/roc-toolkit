/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/control_writer.h"
#include "test_helpers/frame_reader.h"
#include "test_helpers/packet_writer.h"

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/time.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_rtp/encoding_map.h"

// This file contains tests for ReceiverSource. ReceiverSource can be seen as a big
// composite processor (consisting of chanined smaller processors) that transforms
// network packets into audio frames. Typically, network thread writes packets into
// ReceiverSource, and sound card thread read frames from it.
//
// Each test in this file prepares a sequence of input packets and checks what sequence
// of output frames receiver produces in response. Each test checks one aspect of
// pipeline behavior, e.g. handling packet reordering, recovering lost packets, mixing
// multiple sessions, etc.
//
// The tests mostly use three helper classes:
//  - test::PacketWriter - to produce source (RTP) and repair (FEC) packets
//  - test::ControlWriter - to produce control packets (RTCP)
//  - test::FrameReader - to retrieve and validate audio frames
//
// test::PacketWriter and test::ControlWriter simulate remote sender that produces
// packets, and test::FrameReader simulates local sound card that consumes frames.

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

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    Latency = SamplesPerPacket * 8,
    MinLatency = -Latency * 100,
    MaxLatency = +Latency * 100,
    Timeout = Latency * 13,
    Warmup = Latency,

    ManyPackets = Latency / SamplesPerPacket * 10,

    MaxSnJump = ManyPackets * 5,
    MaxTsJump = ManyPackets * 7 * SamplesPerPacket
};

core::HeapArena arena;
core::BufferFactory<audio::sample_t> sample_buffer_factory(arena, MaxBufSize);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, MaxBufSize);
packet::PacketFactory packet_factory(arena);
rtp::EncodingMap encoding_map(arena);

ReceiverSlot* create_slot(ReceiverSource& source) {
    ReceiverSlot* slot = source.create_slot();
    CHECK(slot);
    return slot;
}

packet::IWriter* create_transport_endpoint(ReceiverSlot* slot,
                                           address::Interface iface,
                                           address::Protocol proto) {
    CHECK(slot);
    ReceiverEndpoint* endpoint =
        slot->add_endpoint(iface, proto, address::SocketAddr(), NULL);
    CHECK(endpoint);
    return &endpoint->inbound_writer();
}

packet::IWriter* create_control_endpoint(ReceiverSlot* slot,
                                         address::Interface iface,
                                         address::Protocol proto,
                                         packet::IWriter& outbound_writer) {
    CHECK(slot);
    ReceiverEndpoint* endpoint =
        slot->add_endpoint(iface, proto, address::SocketAddr(), &outbound_writer);
    CHECK(endpoint);
    return &endpoint->inbound_writer();
}

} // namespace

TEST_GROUP(receiver_source) {
    audio::SampleSpec packet_sample_spec;
    audio::SampleSpec output_sample_spec;

    packet::stream_source_t src_id1;
    packet::stream_source_t src_id2;

    address::SocketAddr src_addr1;
    address::SocketAddr src_addr2;

    address::SocketAddr dst_addr1;
    address::SocketAddr dst_addr2;

    address::Protocol proto1;
    address::Protocol proto2;

    ReceiverConfig make_custom_config(int target_latency, int min_latency,
                                      int max_latency, int watchdog_timeout,
                                      int watchdog_warmup) {
        ReceiverConfig config;

        config.common.output_sample_spec = output_sample_spec;

        config.common.enable_timing = false;
        config.common.enable_profiling = true;

        config.default_session.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        config.default_session.latency.tuner_profile = audio::LatencyTunerProfile_Intact;
        config.default_session.latency.target_latency =
            target_latency * core::Second / (int)output_sample_spec.sample_rate();
        config.default_session.latency.min_latency =
            min_latency * core::Second / (int)output_sample_spec.sample_rate();
        config.default_session.latency.max_latency =
            max_latency * core::Second / (int)output_sample_spec.sample_rate();

        config.default_session.watchdog.no_playback_timeout =
            watchdog_timeout * core::Second / (int)output_sample_spec.sample_rate();
        config.default_session.watchdog.warmup_duration =
            watchdog_warmup * core::Second / (int)output_sample_spec.sample_rate();

        config.common.rtp_filter.max_sn_jump = MaxSnJump;
        config.common.rtp_filter.max_ts_jump =
            MaxTsJump * core::Second / (int)output_sample_spec.sample_rate();

        return config;
    }

    ReceiverConfig make_default_config() {
        return make_custom_config(Latency, MinLatency, MaxLatency, Timeout, Warmup);
    }

    void init(int output_sample_rate, audio::ChannelMask output_channels,
              int packet_sample_rate, audio::ChannelMask packet_channels) {
        output_sample_spec.set_sample_rate((size_t)output_sample_rate);
        output_sample_spec.set_sample_format(audio::SampleFormat_Pcm);
        output_sample_spec.set_pcm_format(audio::Sample_RawFormat);
        output_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        output_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        output_sample_spec.channel_set().set_channel_mask(output_channels);

        packet_sample_spec.set_sample_rate((size_t)packet_sample_rate);
        packet_sample_spec.set_sample_format(audio::SampleFormat_Pcm);
        packet_sample_spec.set_pcm_format(audio::PcmFormat_SInt16_Be);
        packet_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        packet_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        packet_sample_spec.channel_set().set_channel_mask(packet_channels);

        src_id1 = 111;
        src_id2 = 222;

        src_addr1 = test::new_address(1);
        src_addr2 = test::new_address(2);

        dst_addr1 = test::new_address(3);
        dst_addr2 = test::new_address(4);

        proto1 = address::Proto_RTP;
        proto2 = address::Proto_RTP;
    }
};

TEST(receiver_source, no_sessions) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
    }
}

TEST(receiver_source, one_session) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, one_session_long_run) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, NumIterations = 10 };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (size_t np = 0; np < ManyPackets; np++) {
            for (size_t nf = 0; nf < FramesPerPacket; nf++) {
                receiver.refresh(frame_reader.refresh_ts());
                frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

                UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
            }

            packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        }
    }
}

// Check how receiver accumulates packets in jitter buffer
// before starting playback.
TEST(receiver_source, initial_latency) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

// Timeout expires during initial latency accumulation.
TEST(receiver_source, initial_latency_timeout) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < (Latency + Timeout) / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    receiver.refresh(frame_reader.refresh_ts());
    frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Timeout expires during playback.
TEST(receiver_source, timeout) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    while (receiver.num_sessions() != 0) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
    }
}

// Checks that receiver can work with latency longer than timeout.
TEST(receiver_source, timeout_smaller_than_latency) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        LargeLatency = Timeout * 5,
        LargeWarmup = LargeLatency
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(
        make_custom_config(LargeLatency, MinLatency, MaxLatency, Timeout, LargeWarmup),
        encoding_map, packet_factory, byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < LargeLatency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    for (size_t np = 0; np < LargeLatency / SamplesPerPacket - 1; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    for (size_t np = 0; np < Timeout / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    receiver.refresh(frame_reader.refresh_ts());
    frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Latency goes below minimum during playback.
TEST(receiver_source, min_latency) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        SmallMinLatency = Latency / 2,
        LargeMaxLatency = Latency * 100,
        LargeTimeout = Latency * 100
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_custom_config(Latency, SmallMinLatency, LargeMaxLatency,
                                               LargeTimeout, Warmup),
                            encoding_map, packet_factory, byte_buffer_factory,
                            sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < (Latency - SmallMinLatency) / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerFrame, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Latency goes above maximum during playback.
TEST(receiver_source, max_latency) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        LargeMinLatency = -Latency * 100,
        SmallMaxLatency = Latency * 3 / 2,
        LargeTimeout = Latency * 100,
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_custom_config(Latency, LargeMinLatency, SmallMaxLatency,
                                               LargeTimeout, Warmup),
                            encoding_map, packet_factory, byte_buffer_factory,
                            sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    for (size_t np = 0; np < (SmallMaxLatency - Latency) / SamplesPerPacket + 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerFrame, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Check how receiver trims incoming queue if initially it receives more
// packets than configured jitter buffer size.
TEST(receiver_source, initial_trim) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency * 3 / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    frame_reader.set_offset(Latency * 2);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, two_sessions_synchronous) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr1, PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id2,
                                      src_addr2, dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_overlapping) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    test::PacketWriter packet_writer2(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id2,
                                      src_addr2, dst_addr1, PayloadType_Ch2);

    packet_writer2.set_offset(packet_writer1.offset() - Latency);
    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_two_endpoints) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot1 = create_slot(receiver);
    CHECK(slot1);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot1, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    ReceiverSlot* slot2 = create_slot(receiver);
    CHECK(slot2);

    packet::IWriter* endpoint2_writer =
        create_transport_endpoint(slot2, address::Iface_AudioSource, proto2);
    CHECK(endpoint2_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr1, PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint2_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id2,
                                      src_addr2, dst_addr2, PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_same_address_same_stream) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr1, PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr2, PayloadType_Ch2);

    packet_writer1.set_source(11);
    packet_writer2.set_source(11);

    packet_writer2.set_offset(77);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_same_address_different_streams) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr1, PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr2, PayloadType_Ch2);

    packet_writer1.set_source(11);
    packet_writer2.set_source(22);

    packet_writer2.set_offset(77);
    packet_writer2.set_seqnum(5);
    packet_writer2.set_timestamp(5 * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, seqnum_overflow) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.set_seqnum(packet::seqnum_t(-1) - ManyPackets / 2);
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, seqnum_small_jump) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, SmallJump = 5 };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + SmallJump);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Receiver should terminate session if seqnum jumped too far.
TEST(receiver_source, seqnum_large_jump) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + MaxSnJump);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    while (receiver.num_sessions() != 0) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
    }
}

TEST(receiver_source, seqnum_reorder) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        ReorderWindow = Latency / SamplesPerPacket
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    size_t pos = 0;

    for (size_t ni = 0; ni < ManyPackets / ReorderWindow; ni++) {
        if (pos >= Latency / SamplesPerPacket) {
            for (size_t nf = 0; nf < ReorderWindow * FramesPerPacket; nf++) {
                receiver.refresh(frame_reader.refresh_ts());
                frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
            }
        }

        for (ssize_t np = ReorderWindow - 1; np >= 0; np--) {
            packet_writer.shift_to(pos + size_t(np), SamplesPerPacket);
            packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        }

        pos += ReorderWindow;
    }
}

TEST(receiver_source, seqnum_late) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, DelayedPackets = 5 };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);
    packet_writer.shift_to(Latency / SamplesPerPacket + DelayedPackets, SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.shift_to(Latency / SamplesPerPacket, SamplesPerPacket);
    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
    }

    receiver.refresh(frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
}

TEST(receiver_source, timestamp_overflow) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.set_timestamp(packet::stream_timestamp_t(-1)
                                - ManyPackets * SamplesPerPacket / 2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_small_jump) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, ShiftedPackets = 5 };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + ShiftedPackets * SamplesPerPacket);
    packet_writer.set_offset(Latency + ShiftedPackets * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ShiftedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Receiver should terminate session if RTP timestamp jumped too far.
TEST(receiver_source, timestamp_large_jump) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + MaxTsJump);
    packet_writer.set_offset(Latency + MaxTsJump);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    while (receiver.num_sessions() != 0) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
    }
}

// Check how receiver handles packets that are overlapping accoring
// to their RTP timestamps.
TEST(receiver_source, timestamp_overlap) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        OverlappedSamples = SamplesPerPacket / 2
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency - OverlappedSamples);
    packet_writer.set_offset(Latency - OverlappedSamples);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_reorder) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (ssize_t np = Latency / SamplesPerPacket - 1; np >= 0; np--) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        packet_writer.set_offset(Latency + size_t(np) * SamplesPerPacket);

        packet_writer.set_timestamp(
            packet::stream_timestamp_t(Latency + size_t(np) * SamplesPerPacket));

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_offset(Latency * 2);
    packet_writer.set_timestamp(Latency * 2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_late) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, DelayedPackets = 5 };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + DelayedPackets * SamplesPerPacket);
    packet_writer.set_offset(Latency + DelayedPackets * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_timestamp(Latency);
    packet_writer.set_offset(Latency);

    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
    }

    receiver.refresh(frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
}

// Packets smaller than frame.
TEST(receiver_source, packet_size_small) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,
        ManySmallPackets = Latency / SamplesPerSmallPacket * 10
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerSmallPacket, SamplesPerSmallPacket,
                                packet_sample_spec);

    for (size_t nf = 0; nf < ManySmallPackets / SmallPacketsPerFrame; nf++) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        for (size_t np = 0; np < SmallPacketsPerFrame; np++) {
            packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        }
    }
}

// Packets larger than frame.
TEST(receiver_source, packet_size_large) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,
        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,
        ManyLargePackets = Latency / SamplesPerLargePacket * 10
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerLargePacket, SamplesPerLargePacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyLargePackets; np++) {
        for (size_t nf = 0; nf < FramesPerLargePacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);
    }
}

TEST(receiver_source, packet_size_variable) {
    enum {
        Rate = SampleRate,
        Chans = Chans_Stereo,

        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,

        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,

        SamplesPerTwoPackets = (SamplesPerSmallPacket + SamplesPerLargePacket),

        NumIterations = Latency / SamplesPerTwoPackets * 10
    };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    size_t available = 0;

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (; available >= Latency; available -= SamplesPerFrame) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);

        available += SamplesPerTwoPackets;
    }
}

// Receiver should ignore corrupted packets and don't create session.
TEST(receiver_source, corrupted_packets_new_session) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.set_corrupt(true);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Receiver should ignore corrupted packets and don't pass them to session.
TEST(receiver_source, corrupted_packets_existing_session) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);
    packet_writer.set_corrupt(true);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_corrupt(false);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Packets are stereo, receiver produces mono.
TEST(receiver_source, channel_mapping_stereo_to_mono) {
    enum { Rate = SampleRate, OutputChans = Chans_Mono, PacketChans = Chans_Stereo };

    init(Rate, OutputChans, Rate, PacketChans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Packets are mono, receiver produces stereo.
TEST(receiver_source, channel_mapping_mono_to_stereo) {
    enum { Rate = SampleRate, OutputChans = Chans_Stereo, PacketChans = Chans_Mono };

    init(Rate, OutputChans, Rate, PacketChans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Packets have one rate, receiver produces different rate.
TEST(receiver_source, sample_rate_mapping) {
    enum { OutputRate = 48000, PacketRate = 44100, Chans = Chans_Stereo };

    init(OutputRate, Chans, PacketRate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_nonzero_samples(SamplesPerFrame * OutputRate / PacketRate
                                                  / output_sample_spec.num_channels()
                                                  * output_sample_spec.num_channels(),
                                              output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// When there are no control packets, receiver always sets CTS of frames to zero.
TEST(receiver_source, timestamp_mapping_no_control_packets) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, address::Proto_RTP);
    CHECK(transport_endpoint);

    packet::Queue control_outbound_queue;
    packet::IWriter* control_endpoint = create_control_endpoint(
        slot, address::Iface_AudioControl, address::Proto_RTCP, control_outbound_queue);
    CHECK(control_endpoint);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    // Expect no CTS.
    const core::nanoseconds_t capture_ts_base = -1;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts(capture_ts_base));
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec,
                                      capture_ts_base);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// When there is one control packet, receiver sets CTS of frames according
// to received mapping.
TEST(receiver_source, timestamp_mapping_one_control_packet) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, address::Proto_RTP);
    CHECK(transport_endpoint);

    packet::Queue control_outbound_queue;
    packet::IWriter* control_endpoint = create_control_endpoint(
        slot, address::Iface_AudioControl, address::Proto_RTCP, control_outbound_queue);
    CHECK(control_endpoint);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory,
                                       byte_buffer_factory, src_addr1, dst_addr2);

    control_writer.set_local_source(src_id1);

    const core::nanoseconds_t capture_ts_base = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            // For first packet, expect no CTS.
            // Then, after control packet is delivered, expect valid CTS.
            core::nanoseconds_t expect_ts_base = -1;
            if (np != 0) {
                expect_ts_base = capture_ts_base;
            }

            receiver.refresh(frame_reader.refresh_ts(capture_ts_base));
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec,
                                      expect_ts_base);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        // After first transport packet, send one control packet.
        if (np == 0) {
            control_writer.write_sender_report(packet::unix_2_ntp(capture_ts_base),
                                               rtp_base);
        }
    }
}

// When there are regular control packets, receiver updates CTS of frames according
// to received mapping.
TEST(receiver_source, timestamp_mapping_periodic_control_packets) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, address::Proto_RTP);
    CHECK(transport_endpoint);

    packet::Queue control_outbound_queue;
    packet::IWriter* control_endpoint = create_control_endpoint(
        slot, address::Iface_AudioControl, address::Proto_RTCP, control_outbound_queue);
    CHECK(control_endpoint);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory,
                                       byte_buffer_factory, src_addr1, dst_addr2);

    control_writer.set_local_source(src_id1);

    const core::nanoseconds_t capture_ts_step = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        const core::nanoseconds_t capture_ts_base = capture_ts_step * ((int)np + 1);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            // For first packet, expect no CTS.
            // Then, after control packet is delivered, expect valid CTS.
            core::nanoseconds_t expect_ts_base = -1;
            if (np != 0) {
                expect_ts_base = capture_ts_base - capture_ts_step;
            }

            receiver.refresh(frame_reader.refresh_ts(capture_ts_base));
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec,
                                      expect_ts_base);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        // Regularly deliver control packets.
        control_writer.write_sender_report(packet::unix_2_ntp(capture_ts_base), rtp_base);
    }

    CHECK(control_outbound_queue.size() > 0);
}

// Check CTS when there is also channel conversion and sample rate conversion.
TEST(receiver_source, timestamp_mapping_remixing) {
    enum {
        OutputRate = 48000,
        PacketRate = 44100,
        OutputChans = Chans_Stereo,
        PacketChans = Chans_Mono
    };

    init(OutputRate, OutputChans, PacketRate, PacketChans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, address::Proto_RTP);
    CHECK(transport_endpoint);

    packet::Queue control_outbound_queue;
    packet::IWriter* control_endpoint = create_control_endpoint(
        slot, address::Iface_AudioControl, address::Proto_RTCP, control_outbound_queue);
    CHECK(control_endpoint);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch1);

    test::ControlWriter control_writer(*control_endpoint, packet_factory,
                                       byte_buffer_factory, src_addr1, dst_addr2);

    control_writer.set_local_source(src_id1);

    const core::nanoseconds_t unix_base = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    const size_t frame_size = SamplesPerFrame * OutputRate / PacketRate
        / output_sample_spec.num_channels() * output_sample_spec.num_channels();
    audio::sample_t frame_data[MaxBufSize];
    size_t frame_num = 0;
    core::nanoseconds_t first_ts = 0;

    core::nanoseconds_t cur_time = 2000000000000000;

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(cur_time);
            cur_time += output_sample_spec.samples_overall_2_ns(frame_size);

            audio::Frame frame(frame_data, frame_size);
            CHECK(receiver.read(frame));

            if (!first_ts && frame.capture_timestamp()) {
                first_ts = frame.capture_timestamp();

                CHECK(first_ts >= unix_base);
                CHECK(first_ts < unix_base + core::Millisecond * 10);
            }

            if (first_ts) {
                const core::nanoseconds_t expected_capture_ts = first_ts
                    + output_sample_spec.samples_overall_2_ns(frame_num * frame_size);

                test::expect_capture_timestamp(
                    expected_capture_ts, frame.capture_timestamp(), output_sample_spec,
                    test::TimestampEpsilonSmpls);

                frame_num++;
            }
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        if (np == 0) {
            control_writer.write_sender_report(packet::unix_2_ntp(unix_base), rtp_base);
        }
    }

    CHECK(first_ts);
}

// Check receiver metrics for multiple remote participants (senders).
TEST(receiver_source, metrics_participants) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, MaxParties = 10 };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    {
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = MaxParties;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        UNSIGNED_LONGS_EQUAL(0, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(0, party_metrics_size);
    }

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    {
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = MaxParties;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        CHECK(slot_metrics.source_id != 0);
        UNSIGNED_LONGS_EQUAL(0, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(0, party_metrics_size);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);

        {
            ReceiverSlotMetrics slot_metrics;
            ReceiverParticipantMetrics party_metrics[MaxParties];
            size_t party_metrics_size = MaxParties;

            slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

            CHECK(slot_metrics.source_id != 0);
            UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

            CHECK(party_metrics[0].latency.niq_latency != 0);
            CHECK(party_metrics[0].latency.e2e_latency == 0);
        }
    }

    test::PacketWriter packet_writer2(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id2,
                                      src_addr2, dst_addr1, PayloadType_Ch2);

    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);

        {
            ReceiverSlotMetrics slot_metrics;
            ReceiverParticipantMetrics party_metrics[MaxParties];
            size_t party_metrics_size = MaxParties;

            slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

            CHECK(slot_metrics.source_id != 0);
            UNSIGNED_LONGS_EQUAL(2, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(2, party_metrics_size);

            CHECK(party_metrics[0].latency.niq_latency != 0);
            CHECK(party_metrics[0].latency.e2e_latency == 0);

            CHECK(party_metrics[1].latency.niq_latency != 0);
            CHECK(party_metrics[1].latency.e2e_latency == 0);
        }
    }
}

// Check how receiver returns metrics if provided buffer for metrics
// is smaller than needed.
TEST(receiver_source, metrics_truncation) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, MaxParties = 10 };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer1(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id1,
                                      src_addr1, dst_addr1, PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, byte_buffer_factory, src_id2,
                                      src_addr2, dst_addr1, PayloadType_Ch2);

    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        receiver.refresh(frame_reader.refresh_ts());
        frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());

    { // metrics_size=0 num_participants=2
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = 0;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        UNSIGNED_LONGS_EQUAL(2, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(0, party_metrics_size);
    }

    { // metrics_size=1 num_participants=2
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = 1;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        CHECK(slot_metrics.source_id != 0);
        UNSIGNED_LONGS_EQUAL(2, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

        CHECK(party_metrics[0].latency.niq_latency > 0);
        CHECK(party_metrics[1].latency.niq_latency == 0);
    }

    { // metrics_size=2 num_participants=2
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = 2;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        CHECK(slot_metrics.source_id != 0);
        UNSIGNED_LONGS_EQUAL(2, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(2, party_metrics_size);

        CHECK(party_metrics[0].latency.niq_latency > 0);
        CHECK(party_metrics[1].latency.niq_latency > 0);
        CHECK(party_metrics[2].latency.niq_latency == 0);
    }

    { // metrics_size=3 num_participants=2
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = 3;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        CHECK(slot_metrics.source_id != 0);
        UNSIGNED_LONGS_EQUAL(2, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(2, party_metrics_size);

        CHECK(party_metrics[0].latency.niq_latency > 0);
        CHECK(party_metrics[1].latency.niq_latency > 0);
        CHECK(party_metrics[2].latency.niq_latency == 0);
    }
}

// Check how receiver computes packet metrics:
// total_packets, lost_packets, ext_first_seqnum, ext_last_seqnum
IGNORE_TEST(receiver_source, metrics_packet_counters) {
    // TODO(gh-688): implement test
}

// Check how receiver computes jitter metric.
IGNORE_TEST(receiver_source, metrics_jitter) {
    // TODO(gh-688): implement test
}

// Check how receiver computes niq_latency metric (network incoming queue size).
TEST(receiver_source, metrics_niq_latency) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, MaxParties = 10 };

    init(Rate, Chans, Rate, Chans);

    const core::nanoseconds_t virtual_niq_latency =
        output_sample_spec.samples_per_chan_2_ns(Latency);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            receiver.refresh(frame_reader.refresh_ts());
            frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, output_sample_spec);

        {
            ReceiverSlotMetrics slot_metrics;
            ReceiverParticipantMetrics party_metrics[MaxParties];
            size_t party_metrics_size = MaxParties;

            slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

            CHECK(slot_metrics.source_id != 0);
            UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

            DOUBLES_EQUAL(virtual_niq_latency, party_metrics[0].latency.niq_latency,
                          core::Millisecond * 5);
        }
    }
}

// Check how receiver computes e2e_latency metric (estimated end-to-end latency).
// This metrics requires control packets exchange.
TEST(receiver_source, metrics_e2e_latency) {
    enum { Rate = SampleRate, Chans = Chans_Stereo, MaxParties = 10 };

    init(Rate, Chans, Rate, Chans);

    const core::nanoseconds_t virtual_e2e_latency = core::Millisecond * 555;

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(transport_endpoint);

    packet::Queue control_outbound_queue;
    packet::IWriter* control_endpoint = create_control_endpoint(
        slot, address::Iface_AudioControl, address::Proto_RTCP, control_outbound_queue);
    CHECK(control_endpoint);

    test::FrameReader frame_reader(receiver, sample_buffer_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory,
                                       byte_buffer_factory, src_addr1, dst_addr2);

    control_writer.set_local_source(src_id1);

    const core::nanoseconds_t capture_ts_base = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            // For first packet, expect no CTS.
            // Then, after control packet is delivered, expect valid CTS.
            core::nanoseconds_t expect_ts_base = -1;
            if (np != 0) {
                expect_ts_base = capture_ts_base;
            }

            receiver.refresh(frame_reader.refresh_ts(capture_ts_base));
            frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec,
                                              expect_ts_base);

            if (np != 0) {
                receiver.reclock(frame_reader.last_capture_ts() + virtual_e2e_latency);
            }

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, output_sample_spec);

        {
            ReceiverSlotMetrics slot_metrics;
            ReceiverParticipantMetrics party_metrics[MaxParties];
            size_t party_metrics_size = MaxParties;

            slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

            CHECK(slot_metrics.source_id != 0);
            UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

            if (np != 0) {
                DOUBLES_EQUAL(virtual_e2e_latency, party_metrics[0].latency.e2e_latency,
                              core::Millisecond);
            }
        }

        // After first transport packet, send one control packet.
        if (np == 0) {
            control_writer.write_sender_report(packet::unix_2_ntp(capture_ts_base),
                                               rtp_base);
        }
    }
}

TEST(receiver_source, pipeline_state) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    init(Rate, Chans, Rate, Chans);

    ReceiverSource receiver(make_default_config(), encoding_map, packet_factory,
                            byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(receiver.is_valid());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::PacketWriter packet_writer(arena, *endpoint1_writer, encoding_map,
                                     packet_factory, byte_buffer_factory, src_id1,
                                     src_addr1, dst_addr1, PayloadType_Ch2);

    core::Slice<audio::sample_t> samples = sample_buffer_factory.new_buffer();
    CHECK(samples);
    samples.reslice(0, FramesPerPacket * output_sample_spec.num_channels());

    core::nanoseconds_t cur_time = 1000000000000000;

    CHECK(receiver.state() == sndio::DeviceState_Idle);

    {
        receiver.refresh(cur_time);
        cur_time += output_sample_spec.samples_overall_2_ns(samples.size());

        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);
    }

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    CHECK(receiver.state() == sndio::DeviceState_Active);

    {
        receiver.refresh(cur_time);
        cur_time += output_sample_spec.samples_overall_2_ns(samples.size());

        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);
    }

    for (;;) {
        receiver.refresh(cur_time);
        cur_time += output_sample_spec.samples_overall_2_ns(samples.size());

        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);

        if (receiver.state() == sndio::DeviceState_Idle) {
            break;
        }
    }
}

} // namespace pipeline
} // namespace roc
