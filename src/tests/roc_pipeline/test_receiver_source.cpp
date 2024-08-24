/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/control_reader.h"
#include "test_helpers/control_writer.h"
#include "test_helpers/frame_reader.h"
#include "test_helpers/packet_writer.h"

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_core/time.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_rtp/encoding_map.h"
#include "roc_stat/mov_aggregate.h"

// This file contains tests for ReceiverSource. ReceiverSource can be seen as a big
// composite processor (consisting of chained smaller processors) that transforms
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

const audio::PcmSubformat Format_Raw = audio::PcmSubformat_Raw;
const audio::PcmSubformat Format_S16_Be = audio::PcmSubformat_SInt16_Be;
const audio::PcmSubformat Format_S16_Ne = audio::PcmSubformat_SInt16;
const audio::PcmSubformat Format_S32_Ne = audio::PcmSubformat_SInt32;

const rtp::PayloadType PayloadType_Ch1 = rtp::PayloadType_L16_Mono;
const rtp::PayloadType PayloadType_Ch2 = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 600,

    SampleRate = 44100,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    SourcePacketsInBlock = 5,
    RepairPacketsInBlock = 4,

    Latency = SamplesPerPacket * 8,
    LatencyTolerance = Latency * 100,
    Timeout = Latency * 13,
    Warmup = Latency,

    ReportInterval = SamplesPerPacket * 10,
    ReportTimeout = Timeout * 2,

    ManyPackets = Latency / SamplesPerPacket * 10,
    ManyReports = 20,

    JitterMeterWindow = ManyPackets * 10,

    MaxSnJump = ManyPackets * 5,
    MaxTsJump = ManyPackets * 7 * SamplesPerPacket
};

core::HeapArena arena;

core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);
core::SlabPool<core::Buffer>
    packet_buffer_pool("packet_buffer_pool", arena, sizeof(core::Buffer) + MaxBufSize);

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer>
    frame_buffer_pool("frame_buffer_pool",
                      arena,
                      sizeof(core::Buffer) + MaxBufSize * sizeof(audio::sample_t));

packet::PacketFactory packet_factory(packet_pool, packet_buffer_pool);
audio::FrameFactory frame_factory(frame_pool, frame_buffer_pool);

audio::ProcessorMap processor_map(arena);
rtp::EncodingMap encoding_map(arena);

ReceiverSlot* create_slot(ReceiverSource& source) {
    ReceiverSlotConfig slot_config;
    ReceiverSlot* slot = source.create_slot(slot_config);
    CHECK(slot);
    return slot;
}

packet::IWriter* create_transport_endpoint(ReceiverSlot* slot,
                                           address::Interface iface,
                                           address::Protocol proto,
                                           const address::SocketAddr& inbound_addr) {
    CHECK(slot);
    ReceiverEndpoint* endpoint = slot->add_endpoint(iface, proto, inbound_addr, NULL);
    CHECK(endpoint);
    packet::IWriter* writer = &endpoint->inbound_writer();
    CHECK(writer);
    return writer;
}

packet::IWriter* create_control_endpoint(ReceiverSlot* slot,
                                         address::Interface iface,
                                         address::Protocol proto,
                                         const address::SocketAddr& inbound_addr,
                                         packet::IWriter& outbound_writer) {
    CHECK(slot);
    ReceiverEndpoint* endpoint =
        slot->add_endpoint(iface, proto, inbound_addr, &outbound_writer);
    CHECK(endpoint);
    packet::IWriter* writer = &endpoint->inbound_writer();
    CHECK(writer);
    return writer;
}

void refresh_source(ReceiverSource& receiver_source, core::nanoseconds_t refresh_ts) {
    LONGS_EQUAL(status::StatusOK, receiver_source.refresh(refresh_ts, NULL));
}

void read_into_frame(audio::IFrameReader& reader,
                     audio::Frame& frame,
                     const audio::SampleSpec& sample_spec,
                     size_t n_samples) {
    CHECK(n_samples % sample_spec.num_channels() == 0);

    LONGS_EQUAL(
        status::StatusOK,
        reader.read(frame, n_samples / sample_spec.num_channels(), audio::ModeHard));

    if (sample_spec.is_raw()) {
        CHECK(frame.is_raw());
        CHECK(frame.raw_samples());
        UNSIGNED_LONGS_EQUAL(n_samples, frame.num_raw_samples());
    } else {
        CHECK(!frame.is_raw());
    }

    CHECK(frame.bytes());
    UNSIGNED_LONGS_EQUAL(n_samples / sample_spec.num_channels(), frame.duration());
    UNSIGNED_LONGS_EQUAL(sample_spec.stream_timestamp_2_bytes(packet::stream_timestamp_t(
                             n_samples / sample_spec.num_channels())),
                         frame.num_bytes());
}

audio::FramePtr read_frame(audio::IFrameReader& reader,
                           const audio::SampleSpec& sample_spec,
                           size_t n_samples) {
    audio::FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    read_into_frame(reader, *frame, sample_spec, n_samples);

    return frame;
}

packet::PacketPtr read_packet(packet::IReader& reader) {
    packet::PacketPtr pp;
    const status::StatusCode code = reader.read(pp, packet::ModeFetch);
    if (code == status::StatusOK) {
        CHECK(pp);
        return pp;
    }
    LONGS_EQUAL(status::StatusDrain, code);
    CHECK(!pp);
    return NULL;
}

void write_packet(packet::IWriter& writer, const packet::PacketPtr& pp) {
    LONGS_EQUAL(status::StatusOK, writer.write(pp));
}

core::nanoseconds_t get_niq_latency(ReceiverSlot& receiver_slot) {
    ReceiverSlotMetrics slot_metrics;
    ReceiverParticipantMetrics party_metrics;
    size_t party_metrics_size = 1;

    receiver_slot.get_metrics(slot_metrics, &party_metrics, &party_metrics_size);

    CHECK(slot_metrics.source_id != 0);
    UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);

    return party_metrics.latency.niq_latency;
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

    address::SocketAddr multicast_addr1;
    address::SocketAddr multicast_addr2;

    address::Protocol proto1;
    address::Protocol proto2;

    address::Protocol source_proto;
    address::Protocol repair_proto;

    packet::FecScheme fec_scheme;
    fec::BlockWriterConfig fec_config;

    audio::PlcBackend plc_backend;

    ReceiverSourceConfig make_custom_config(int target_latency, int latency_tolerance,
                                            int watchdog_timeout, int watchdog_warmup) {
        ReceiverSourceConfig config;

        config.common.output_sample_spec = output_sample_spec;

        config.common.enable_cpu_clock = false;
        config.common.enable_profiling = true;

        config.session_defaults.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        config.session_defaults.latency.tuner_profile = audio::LatencyTunerProfile_Intact;
        config.session_defaults.latency.target_latency =
            target_latency * core::Second / (int)output_sample_spec.sample_rate();
        config.session_defaults.latency.latency_tolerance =
            latency_tolerance * core::Second / (int)output_sample_spec.sample_rate();

        config.session_defaults.watchdog.no_playback_timeout =
            watchdog_timeout * core::Second / (int)output_sample_spec.sample_rate();
        config.session_defaults.watchdog.warmup_duration =
            watchdog_warmup * core::Second / (int)output_sample_spec.sample_rate();

        config.session_defaults.plc.backend = plc_backend;

        config.session_defaults.jitter_meter.jitter_window = JitterMeterWindow;

        config.common.rtcp.report_interval = ReportInterval * core::Second / SampleRate;
        config.common.rtcp.inactivity_timeout = ReportTimeout * core::Second / SampleRate;

        config.common.rtp_filter.max_sn_jump = MaxSnJump;
        config.common.rtp_filter.max_ts_jump =
            MaxTsJump * core::Second / (int)output_sample_spec.sample_rate();

        return config;
    }

    ReceiverSourceConfig make_default_config() {
        return make_custom_config(Latency, LatencyTolerance, Timeout, Warmup);
    }

    ReceiverSourceConfig make_adaptive_config(
        core::nanoseconds_t start_latency, core::nanoseconds_t min_target_latency,
        core::nanoseconds_t max_target_latency, core::nanoseconds_t latency_tolerance,
        core::nanoseconds_t reaction) {
        ReceiverSourceConfig config =
            make_custom_config(Latency, LatencyTolerance, Timeout, Warmup);

        if (processor_map.has_resampler_backend(audio::ResamplerBackend_SpeexDec)) {
            config.session_defaults.resampler.backend = audio::ResamplerBackend_SpeexDec;
        } else {
            config.session_defaults.resampler.backend = audio::ResamplerBackend_Auto;
        }
        config.session_defaults.resampler.profile = audio::ResamplerProfile_Low;

        config.session_defaults.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        config.session_defaults.latency.tuner_profile =
            audio::LatencyTunerProfile_Gradual;

        config.session_defaults.latency.target_latency = 0;
        config.session_defaults.latency.latency_tolerance = latency_tolerance;

        config.session_defaults.latency.start_target_latency = start_latency;
        config.session_defaults.latency.min_target_latency = min_target_latency;
        config.session_defaults.latency.max_target_latency = max_target_latency;

        config.session_defaults.latency.starting_timeout = reaction;
        config.session_defaults.latency.cooldown_dec_timeout = reaction;
        config.session_defaults.latency.cooldown_inc_timeout = reaction;

        config.session_defaults.freq_est.stability_duration_criteria = reaction;
        config.session_defaults.freq_est.P = 1e-6 * 1.5;
        config.session_defaults.freq_est.I = 5e-9 * 1.5;

        return config;
    }

    void init_with_specs(int output_sample_rate, audio::ChannelMask output_channels,
                         audio::PcmSubformat output_format, int packet_sample_rate,
                         audio::ChannelMask packet_channels,
                         audio::PcmSubformat packet_format) {
        output_sample_spec.set_format(audio::Format_Pcm);
        output_sample_spec.set_pcm_subformat(output_format);
        output_sample_spec.set_sample_rate((size_t)output_sample_rate);
        output_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        output_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        output_sample_spec.channel_set().set_mask(output_channels);

        packet_sample_spec.set_format(audio::Format_Pcm);
        packet_sample_spec.set_pcm_subformat(packet_format);
        packet_sample_spec.set_sample_rate((size_t)packet_sample_rate);
        packet_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        packet_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        packet_sample_spec.channel_set().set_mask(packet_channels);

        src_id1 = 111;
        src_id2 = 222;

        src_addr1 = test::new_address(11);
        src_addr2 = test::new_address(12);

        dst_addr1 = test::new_address(21);
        dst_addr2 = test::new_address(22);

        CHECK(multicast_addr1.set_host_port_auto("224.0.0.1", 1111));
        CHECK(multicast_addr2.set_host_port_auto("224.0.0.1", 2222));

        proto1 = address::Proto_RTP;
        proto2 = address::Proto_RTP;

        source_proto = address::Proto_RTP_RS8M_Source;
        repair_proto = address::Proto_RS8M_Repair;

        fec_scheme = packet::FEC_ReedSolomon_M8;

        fec_config.n_source_packets = SourcePacketsInBlock;
        fec_config.n_repair_packets = RepairPacketsInBlock;

        plc_backend = audio::PlcBackend_None;
    }

    void init_with_defaults() {
        init_with_specs(SampleRate, Chans_Stereo, Format_Raw, SampleRate, Chans_Stereo,
                        Format_S16_Be);
    }

    void init_with_plc(audio::PlcBackend backend) {
        init_with_defaults();

        plc_backend = backend;
    }

    bool fec_supported() {
        return fec::CodecMap::instance().has_scheme(fec_scheme);
    }
};

TEST(receiver_source, no_sessions) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    test::FrameReader frame_reader(receiver, frame_factory);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
    }
}

TEST(receiver_source, one_session) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, one_session_long_run) {
    enum { NumIterations = 10 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (size_t np = 0; np < ManyPackets; np++) {
            for (size_t nf = 0; nf < FramesPerPacket; nf++) {
                refresh_source(receiver, frame_reader.refresh_ts());
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
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    frame_reader.set_offset(0);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

// Timeout expires during initial latency accumulation.
TEST(receiver_source, initial_latency_timeout) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < (Latency + Timeout) / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Timeout expires during playback.
TEST(receiver_source, no_playback_timeout) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    while (receiver.num_sessions() != 0) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
    }
}

// Checks that receiver can work with latency longer than timeout.
TEST(receiver_source, no_playback_timeout_smaller_than_latency) {
    enum {
        LargeLatency = Timeout * 5,
        LargeWarmup = LargeLatency,
    };

    init_with_defaults();

    ReceiverSource receiver(
        make_custom_config(LargeLatency, LatencyTolerance, Timeout, LargeWarmup),
        processor_map, encoding_map, packet_pool, packet_buffer_pool, frame_pool,
        frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < LargeLatency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    frame_reader.set_offset(0);

    for (size_t np = 0; np < ManyPackets; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    for (size_t np = 0; np < LargeLatency / SamplesPerPacket - 1; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    for (size_t np = 0; np < Timeout / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Latency goes below `Target-Tolerance` during playback.
TEST(receiver_source, latency_lower_bound) {
    enum {
        SmallTolerance = Latency / 2,
        LargeTimeout = Latency * 100,
    };

    init_with_defaults();

    ReceiverSource receiver(
        make_custom_config(Latency, SmallTolerance, LargeTimeout, Warmup), processor_map,
        encoding_map, packet_pool, packet_buffer_pool, frame_pool, frame_buffer_pool,
        arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    frame_reader.set_offset(0);

    for (size_t np = 0; np < SmallTolerance / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerFrame, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Latency goes above `Target+Tolerance` during playback.
TEST(receiver_source, latency_upper_bound) {
    enum {
        SmallTolerance = Latency * 3 / 2,
        LargeTimeout = Latency * 100,
    };

    init_with_defaults();

    ReceiverSource receiver(
        make_custom_config(Latency, SmallTolerance, LargeTimeout, Warmup), processor_map,
        encoding_map, packet_pool, packet_buffer_pool, frame_pool, frame_buffer_pool,
        arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    frame_reader.set_offset(0);

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    for (size_t np = 0; np < SmallTolerance / SamplesPerPacket + 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }
    }

    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerFrame, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

// Check how receiver trims incoming queue if initially it receives more
// packets than configured jitter buffer size.
TEST(receiver_source, initial_trim) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency * 3 / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    frame_reader.set_offset(Latency * 2);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, two_sessions_synchronous) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id2, src_addr2, dst_addr1,
                                      PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_overlapping) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id2, src_addr2, dst_addr1,
                                      PayloadType_Ch2);

    packet_writer2.set_offset(packet_writer1.offset() - Latency);
    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_two_endpoints) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot1 = create_slot(receiver);
    packet::IWriter* endpoint1_writer =
        create_transport_endpoint(slot1, address::Iface_AudioSource, proto1, dst_addr1);

    ReceiverSlot* slot2 = create_slot(receiver);
    packet::IWriter* endpoint2_writer =
        create_transport_endpoint(slot2, address::Iface_AudioSource, proto2, dst_addr2);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint1_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint2_writer, encoding_map,
                                      packet_factory, src_id2, src_addr2, dst_addr2,
                                      PayloadType_Ch2);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 2, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_same_address_same_stream) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    packet_writer1.set_source(11);
    packet_writer2.set_source(11);

    packet_writer2.set_offset(77);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, two_sessions_same_address_different_streams) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

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
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);
    }
}

TEST(receiver_source, seqnum_wrap) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.set_seqnum(packet::seqnum_t(-1) - ManyPackets / 2);
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, seqnum_small_jump) {
    enum { SmallJump = 5 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + SmallJump);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Receiver should terminate session if seqnum jumped too far.
TEST(receiver_source, seqnum_large_jump) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + MaxSnJump);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    while (receiver.num_sessions() != 0) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
    }
}

TEST(receiver_source, seqnum_reorder) {
    enum { ReorderWindow = Latency / SamplesPerPacket };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    size_t pos = 0;

    for (size_t ni = 0; ni < ManyPackets / ReorderWindow; ni++) {
        if (pos >= Latency / SamplesPerPacket) {
            for (size_t nf = 0; nf < ReorderWindow * FramesPerPacket; nf++) {
                refresh_source(receiver, frame_reader.refresh_ts());
                frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
            }
        }

        for (ssize_t np = ReorderWindow - 1; np >= 0; np--) {
            packet_writer.jump_to(pos + size_t(np), SamplesPerPacket);
            packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        }

        pos += ReorderWindow;
    }
}

TEST(receiver_source, seqnum_late) {
    enum { DelayedPackets = 5 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);
    packet_writer.jump_to(Latency / SamplesPerPacket + DelayedPackets, SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.jump_to(Latency / SamplesPerPacket, SamplesPerPacket);
    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
    }

    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
}

TEST(receiver_source, timestamp_wrap) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.set_timestamp(packet::stream_timestamp_t(-1)
                                - ManyPackets * SamplesPerPacket / 2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_small_jump) {
    enum { ShiftedPackets = 5 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + ShiftedPackets * SamplesPerPacket);
    packet_writer.set_offset(Latency + ShiftedPackets * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ShiftedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Receiver should terminate session if RTP timestamp jumped too far.
TEST(receiver_source, timestamp_large_jump) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + MaxTsJump);
    packet_writer.set_offset(Latency + MaxTsJump);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    while (receiver.num_sessions() != 0) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
    }
}

// Check how receiver handles packets that are overlapping according
// to their RTP timestamps.
TEST(receiver_source, timestamp_overlap) {
    enum { OverlappedSamples = SamplesPerPacket / 2 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency - OverlappedSamples);
    packet_writer.set_offset(Latency - OverlappedSamples);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_reorder) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (ssize_t np = Latency / SamplesPerPacket - 1; np >= 0; np--) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
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
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, timestamp_late) {
    enum { DelayedPackets = 5 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    packet_writer.set_timestamp(Latency + DelayedPackets * SamplesPerPacket);
    packet_writer.set_offset(Latency + DelayedPackets * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.set_timestamp(Latency);
    packet_writer.set_offset(Latency);

    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
    }

    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);
}

// Packets smaller than frame.
TEST(receiver_source, packet_size_small) {
    enum {
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,
        ManySmallPackets = Latency / SamplesPerSmallPacket * 10,
    };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerSmallPacket, SamplesPerSmallPacket,
                                packet_sample_spec);

    for (size_t nf = 0; nf < ManySmallPackets / SmallPacketsPerFrame; nf++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        for (size_t np = 0; np < SmallPacketsPerFrame; np++) {
            packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        }
    }
}

// Packets larger than frame.
TEST(receiver_source, packet_size_large) {
    enum {
        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,
        ManyLargePackets = Latency / SamplesPerLargePacket * 10,
    };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerLargePacket, SamplesPerLargePacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyLargePackets; np++) {
        for (size_t nf = 0; nf < FramesPerLargePacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }
        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);
    }
}

TEST(receiver_source, packet_size_variable) {
    enum {
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,

        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,

        SamplesPerTwoPackets = (SamplesPerSmallPacket + SamplesPerLargePacket),

        NumIterations = Latency / SamplesPerTwoPackets * 10,
    };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    size_t available = 0;

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (; available >= Latency; available -= SamplesPerFrame) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
        }

        packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);

        available += SamplesPerTwoPackets;
    }
}

TEST(receiver_source, variable_size_frames_and_packets) {
    enum {
        SamplesPerSmallFrame = 17,
        SamplesPerLargeFrame = 44,

        SamplesPerSmallPacket = 20,
        SamplesPerLargePacket = 37,

        NumFrames = 100
    };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    size_t wr_samples = 0, rd_samples = 0;

    CHECK(Latency % SamplesPerSmallPacket == 0);

    packet_writer.write_packets(Latency / SamplesPerSmallPacket, SamplesPerSmallPacket,
                                packet_sample_spec);
    wr_samples += Latency;

    for (size_t nf = 0; nf < NumFrames; nf++) {
        while (rd_samples + (SamplesPerSmallFrame + SamplesPerLargeFrame) < wr_samples) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerSmallFrame, 1, output_sample_spec);
            rd_samples += SamplesPerSmallFrame;

            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerLargeFrame, 1, output_sample_spec);
            rd_samples += SamplesPerLargeFrame;
        }

        packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        wr_samples += SamplesPerSmallPacket;

        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);
        wr_samples += SamplesPerLargePacket;
    }
}

TEST(receiver_source, frequent_losses_small_packets) {
    enum {
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,
        ManySmallPackets = Latency / SamplesPerSmallPacket * 10,
        LossFreq = 3,
    };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // initial latency
    packet_writer.write_packets(Latency / SamplesPerSmallPacket, SamplesPerSmallPacket,
                                packet_sample_spec);

    // period with losses
    size_t n_packets = 0;

    for (size_t nf = 0; nf < ManySmallPackets / SmallPacketsPerFrame; nf++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec);

        for (size_t np = 0; np < SmallPacketsPerFrame; np++) {
            n_packets++;
            if (n_packets % LossFreq != 0) {
                packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
            } else {
                packet_writer.skip_packets(1, SamplesPerSmallPacket, packet_sample_spec);
            }
        }
    }

    // period without losses
    for (size_t nf = 0; nf < Latency / SamplesPerFrame; nf++) {
        // losses still possible during Latency samples
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec);

        for (size_t np = 0; np < SmallPacketsPerFrame; np++) {
            packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        }
    }

    for (size_t nf = 0; nf < ManySmallPackets / SmallPacketsPerFrame; nf++) {
        // no losses from now
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

        for (size_t np = 0; np < SmallPacketsPerFrame; np++) {
            packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        }
    }
}

TEST(receiver_source, frequent_losses_variable_size_packets) {
    enum {
        SamplesPerSmallPacket = SamplesPerFrame / 3,
        SamplesPerLargePacket = SamplesPerFrame + SamplesPerFrame / 3,

        SmallPacketLossFreq = 3,
        LargePacketLossFreq = 5,

        NumFrames = 100
    };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    size_t wr_samples = 0, rd_samples = 0;
    size_t n_packets = 0;

    // period with losses
    for (size_t nf = 0; nf < NumFrames; nf++) {
        n_packets++;

        if (wr_samples < Latency || n_packets % SmallPacketLossFreq != 0) {
            packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        } else {
            packet_writer.skip_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        }
        wr_samples += SamplesPerSmallPacket;

        if (wr_samples < Latency || n_packets % LargePacketLossFreq != 0) {
            packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);
        } else {
            packet_writer.skip_packets(1, SamplesPerLargePacket, packet_sample_spec);
        }
        wr_samples += SamplesPerLargePacket;

        while (wr_samples >= Latency && rd_samples + SamplesPerFrame < wr_samples) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_any_samples(SamplesPerFrame, output_sample_spec);
            rd_samples += SamplesPerFrame;
        }
    }

    // transitional period
    for (size_t nf = 0; nf < Latency / SamplesPerFrame; nf++) {
        packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        wr_samples += SamplesPerSmallPacket;

        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);
        wr_samples += SamplesPerLargePacket;

        while (rd_samples + SamplesPerFrame < wr_samples) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_any_samples(SamplesPerFrame, output_sample_spec);
            rd_samples += SamplesPerFrame;
        }
    }

    // period without losses
    for (size_t nf = 0; nf < NumFrames; nf++) {
        packet_writer.write_packets(1, SamplesPerSmallPacket, packet_sample_spec);
        wr_samples += SamplesPerSmallPacket;

        packet_writer.write_packets(1, SamplesPerLargePacket, packet_sample_spec);
        wr_samples += SamplesPerLargePacket;

        while (rd_samples + SamplesPerFrame < wr_samples) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
            rd_samples += SamplesPerFrame;
        }
    }
}

// Receiver should ignore corrupted packets and don't create session.
TEST(receiver_source, corrupted_packets_new_session) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.corrupt_packets(true);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Receiver should ignore corrupted packets and don't pass them to session.
TEST(receiver_source, corrupted_packets_existing_session) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);
    packet_writer.corrupt_packets(true);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    packet_writer.corrupt_packets(false);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 0, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// If a few packets are delayed and delivered later, ensure that pipeline drops
// only those packets which were already played, but can successfully use others.
// See gh-54 for more details.
TEST(receiver_source, delayed_reordered_packets) {
    enum {
        LatencyPackets = Latency / SamplesPerPacket,
        P1 = LatencyPackets + 0,
        P2 = LatencyPackets + 1,
        P3 = LatencyPackets + 2,
        P4 = LatencyPackets + 3,
    };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // initial latency
    packet_writer.write_packets(LatencyPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < LatencyPackets; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
    }

    // deliver P1
    packet_writer.jump_to(P1, SamplesPerPacket);
    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    // deliver P4
    packet_writer.jump_to(P4, SamplesPerPacket);
    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    // read P1
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // read gap instead of P2
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 0, output_sample_spec);

    // deliver P2
    packet_writer.jump_to(P2, SamplesPerPacket);
    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    // deliver P3
    packet_writer.jump_to(P3, SamplesPerPacket);
    packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

    // read P3
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // read P4
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
}

// Check how PLC fills gaps caused by packet losses.
TEST(receiver_source, losses_plc) {
    enum {
        LatencyPackets = Latency / SamplesPerPacket,
        LossFreq = 3,
    };

    init_with_plc(audio::PlcBackend_Beep);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(LatencyPackets, SamplesPerPacket, packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());

            if (np > LatencyPackets && (np - LatencyPackets) % LossFreq != 0) {
                frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
            } else {
                // there are always non-zero samples because PLC fills losses
                frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec);
            }

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        if (np % LossFreq != 0) {
            packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
        } else {
            packet_writer.skip_packets(1, SamplesPerPacket, packet_sample_spec);
        }
    }
}

// Enable FEC, deliver all packets without losses.
TEST(receiver_source, fec_no_losses) {
    init_with_defaults();

    if (!fec_supported()) {
        return;
    }

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* source_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioSource, source_proto, dst_addr1);
    packet::IWriter* repair_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioRepair, repair_proto, dst_addr2);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *source_endpoint_writer,
                                     *repair_endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     dst_addr2, PayloadType_Ch2, fec_scheme, fec_config);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Enable FEC, lose some source packets, and ensure that the original stream is restored.
TEST(receiver_source, fec_lose_source_packets) {
    enum {
        // lose every 3rd source packet
        LossFreq = 3
    };

    init_with_defaults();

    if (!fec_supported()) {
        return;
    }

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* source_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioSource, source_proto, dst_addr1);
    packet::IWriter* repair_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioRepair, repair_proto, dst_addr2);

    packet::FifoQueue source_queue;
    packet::FifoQueue repair_queue;

    packet::PacketPtr pp;
    size_t pp_pos = 0;
    size_t n_lost = 0;

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, source_queue, repair_queue, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     dst_addr2, PayloadType_Ch2, fec_scheme, fec_config);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        while ((pp = read_packet(source_queue))) {
            if (++pp_pos % LossFreq != 0) {
                write_packet(*source_endpoint_writer, pp);
            } else {
                n_lost++;
            }
        }
        while ((pp = read_packet(repair_queue))) {
            write_packet(*repair_endpoint_writer, pp);
        }

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }

    CHECK(n_lost >= ManyPackets / LossFreq);
}

// Enable FEC and lose all repair packets.
TEST(receiver_source, fec_lose_repair_packets) {
    init_with_defaults();

    if (!fec_supported()) {
        return;
    }

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* source_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioSource, source_proto, dst_addr1);
    create_transport_endpoint(slot, address::Iface_AudioRepair, repair_proto, dst_addr2);

    packet::FifoQueue black_hole;

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(
        arena, *source_endpoint_writer, black_hole, encoding_map, packet_factory, src_id1,
        src_addr1, dst_addr1, dst_addr2, PayloadType_Ch2, fec_scheme, fec_config);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// If a few source packets are delayed and delivered later, pipeline should drop only
// those packets which were already played, but should successfully use others.
// See gh-210 for more details.
TEST(receiver_source, fec_delay_source_packets) {
    enum { LatencyPackets = Latency / SamplesPerPacket, InitialBlocks = 2 };

    CHECK((InitialBlocks - 1) * SourcePacketsInBlock * SamplesPerPacket < Latency);
    CHECK(InitialBlocks * SourcePacketsInBlock * SamplesPerPacket > Latency);

    init_with_defaults();

    if (!fec_supported()) {
        return;
    }

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* source_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioSource, source_proto, dst_addr1);
    packet::IWriter* repair_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioRepair, repair_proto, dst_addr2);

    packet::FifoQueue source_queue;
    packet::FifoQueue repair_queue;

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, source_queue, repair_queue, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     dst_addr2, PayloadType_Ch2, fec_scheme, fec_config);

    packet_writer.write_packets(SourcePacketsInBlock * (InitialBlocks + 1),
                                SamplesPerPacket, packet_sample_spec);

    // initial latency
    size_t wr_packets = 0, rd_packets = 0;

    for (size_t n_blk = 0; n_blk < InitialBlocks; n_blk++) {
        for (size_t np = 0; np < SourcePacketsInBlock; np++) {
            packet::PacketPtr pp = read_packet(source_queue);
            CHECK(pp);
            write_packet(*source_endpoint_writer, pp);
            wr_packets++;

            if (wr_packets >= LatencyPackets) {
                refresh_source(receiver, frame_reader.refresh_ts());
                frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
                rd_packets++;
            }
        }

        for (size_t np = 0; np < RepairPacketsInBlock; np++) {
            packet::PacketPtr pp = read_packet(repair_queue);
            CHECK(pp);
            write_packet(*repair_endpoint_writer, pp);
        }
    }

    // read everything that we've accumulated
    while (rd_packets < wr_packets) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
        rd_packets++;
    }

    // get first 4 packets
    LONGS_EQUAL(SourcePacketsInBlock, source_queue.size());
    LONGS_EQUAL(RepairPacketsInBlock, repair_queue.size());

    packet::PacketPtr p1 = read_packet(source_queue);
    packet::PacketPtr p2 = read_packet(source_queue);
    packet::PacketPtr p3 = read_packet(source_queue);
    packet::PacketPtr p4 = read_packet(source_queue);

    // deliver P1
    write_packet(*source_endpoint_writer, p1);
    // deliver P4
    write_packet(*source_endpoint_writer, p4);

    // read P1
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // read gap instead of P2
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 0, output_sample_spec);

    // deliver P2
    write_packet(*source_endpoint_writer, p2);
    // deliver P3
    write_packet(*source_endpoint_writer, p3);

    // read P3
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // read P4
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // deliver remaining packets
    while (packet::PacketPtr pp = read_packet(source_queue)) {
        write_packet(*source_endpoint_writer, pp);
    }

    // read remaining packets
    for (size_t np = 4; np < SourcePacketsInBlock; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
    }
}

// If a few source packets are lost, and repair packets are delayed and delivered
// later, pipeline should drop only those packets which were already played, but
// should successfully use others.
// See gh-210 for more details.
TEST(receiver_source, fec_delay_repair_packets) {
    enum { LatencyPackets = Latency / SamplesPerPacket, InitialBlocks = 2 };

    CHECK((InitialBlocks - 1) * SourcePacketsInBlock * SamplesPerPacket < Latency);
    CHECK(InitialBlocks * SourcePacketsInBlock * SamplesPerPacket > Latency);

    init_with_defaults();

    if (!fec_supported()) {
        return;
    }

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* source_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioSource, source_proto, dst_addr1);
    packet::IWriter* repair_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioRepair, repair_proto, dst_addr2);

    packet::FifoQueue source_queue;
    packet::FifoQueue repair_queue;

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, source_queue, repair_queue, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     dst_addr2, PayloadType_Ch2, fec_scheme, fec_config);

    packet_writer.write_packets(SourcePacketsInBlock * (InitialBlocks + 1),
                                SamplesPerPacket, packet_sample_spec);

    // initial latency
    size_t wr_packets = 0, rd_packets = 0;

    for (size_t n_blk = 0; n_blk < InitialBlocks; n_blk++) {
        for (size_t np = 0; np < SourcePacketsInBlock; np++) {
            packet::PacketPtr pp = read_packet(source_queue);
            CHECK(pp);
            write_packet(*source_endpoint_writer, pp);
            wr_packets++;

            if (wr_packets >= LatencyPackets) {
                refresh_source(receiver, frame_reader.refresh_ts());
                frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
                rd_packets++;
            }
        }

        for (size_t np = 0; np < RepairPacketsInBlock; np++) {
            packet::PacketPtr pp = read_packet(repair_queue);
            CHECK(pp);
            write_packet(*repair_endpoint_writer, pp);
        }
    }

    // read everything that we've accumulated
    while (rd_packets < wr_packets) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
        rd_packets++;
    }

    // get first 4 packets
    LONGS_EQUAL(SourcePacketsInBlock, source_queue.size());
    LONGS_EQUAL(RepairPacketsInBlock, repair_queue.size());

    packet::PacketPtr p1 = read_packet(source_queue);
    packet::PacketPtr p2 = read_packet(source_queue);
    packet::PacketPtr p3 = read_packet(source_queue);
    packet::PacketPtr p4 = read_packet(source_queue);

    // deliver P1
    write_packet(*source_endpoint_writer, p1);
    // deliver P4
    write_packet(*source_endpoint_writer, p4);

    // read P1
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // read gap instead of P2
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 0, output_sample_spec);

    // deliver remaining packets
    while (packet::PacketPtr pp = read_packet(source_queue)) {
        write_packet(*source_endpoint_writer, pp);
    }
    while (packet::PacketPtr pp = read_packet(repair_queue)) {
        write_packet(*repair_endpoint_writer, pp);
    }

    // read P3 (should be repaired)
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // read P4
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

    // read remaining packets
    for (size_t np = 4; np < SourcePacketsInBlock; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
    }
}

TEST(receiver_source, soft_read_one_session) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // initial latency
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        // no packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);

        // add 2 packets
        packet_writer.write_packets(2, SamplesPerPacket, packet_sample_spec);

        // request 0.5 packets, get 0.5 packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket / 2, SamplesPerPacket / 2, 1,
                                       output_sample_spec);

        // request 2 packets, get 1.5 packets
        // (because session has only 1.5 packets remaining)
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket * 2,
                                       SamplesPerPacket * 2 - SamplesPerPacket / 2, 1,
                                       output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver_source, soft_read_two_sessions) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id2, src_addr2, dst_addr1,
                                      PayloadType_Ch2);

    // initial latency
    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 packet_sample_spec);
    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 packet_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 2, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        // no packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);

        // add 2 packets to session 1
        packet_writer1.write_packets(2, SamplesPerPacket, packet_sample_spec);
        // add 1 packet to session 2
        packet_writer2.write_packets(1, SamplesPerPacket, packet_sample_spec);

        // request 0.5 packets, get 0.5 packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket / 2, SamplesPerPacket / 2, 2,
                                       output_sample_spec);

        // request 2 packets, get 0.5 packets
        // (because session 2 has only 0.5 packets remaining)
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket * 2, SamplesPerPacket / 2, 2,
                                       output_sample_spec);

        // add 1 packet to session 2
        packet_writer2.write_packets(1, SamplesPerPacket, packet_sample_spec);

        // request 2 packets, get 1 packet
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket * 2, SamplesPerPacket, 2,
                                       output_sample_spec);

        UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
    }
}

TEST(receiver_source, soft_read_before_after) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // soft read drains when there are no sessions
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);
    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());

    // initial latency
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    // soft read drains before first hard read
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);
    UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());

    // first hard read
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
    UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());

    // now soft reads work
    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket, SamplesPerPacket, 1,
                                       output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    // wait until session terminated by timeout
    while (receiver.num_sessions() != 0) {
        refresh_source(receiver, frame_reader.refresh_ts());
        // soft read drain because there are no samples
        frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);
        // hard read works
        frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);
    }

    // soft read drains because there are no sessions again
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);
    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

TEST(receiver_source, soft_read_fec) {
    init_with_defaults();

    if (!fec_supported()) {
        return;
    }

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* source_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioSource, source_proto, dst_addr1);
    packet::IWriter* repair_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioRepair, repair_proto, dst_addr2);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *source_endpoint_writer,
                                     *repair_endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     dst_addr2, PayloadType_Ch2, fec_scheme, fec_config);

    // initial latency
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        // no packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);

        // add 2 packets
        packet_writer.write_packets(2, SamplesPerPacket, packet_sample_spec);

        // request 0.5 packets, get 0.5 packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket / 2, SamplesPerPacket / 2, 1,
                                       output_sample_spec);

        // request 2 packets, get 1.5 packets
        // (because session has only 1.5 packets remaining)
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket * 2,
                                       SamplesPerPacket * 2 - SamplesPerPacket / 2, 1,
                                       output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver_source, soft_read_delays) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    packet::FifoQueue packet_queue;
    test::PacketWriter packet_writer(arena, packet_queue, encoding_map, packet_factory,
                                     src_id1, src_addr1, dst_addr1, PayloadType_Ch2);

    // initial latency
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);
    while (packet::PacketPtr pp = read_packet(packet_queue)) {
        write_packet(*endpoint_writer, pp);
    }

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        // generate 3 packets
        packet_writer.write_packets(3, SamplesPerPacket, packet_sample_spec);
        packet::PacketPtr p1 = read_packet(packet_queue);
        packet::PacketPtr p2 = read_packet(packet_queue);
        packet::PacketPtr p3 = read_packet(packet_queue);

        // deliver P1 and P3
        write_packet(*endpoint_writer, p1);
        write_packet(*endpoint_writer, p3);

        // request 3 packets, get 1 packet
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket * 3, SamplesPerPacket, 1,
                                       output_sample_spec);

        // request 2 packets, get 0 packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket * 2, 0, 0, output_sample_spec);

        // deliver P2
        write_packet(*endpoint_writer, p2);

        // request 2 packets, get 2 packets
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket * 2, SamplesPerPacket * 2, 1,
                                       output_sample_spec);

        UNSIGNED_LONGS_EQUAL(0, packet_queue.size());
        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver_source, soft_read_delays_fec) {
    enum { LatencyPackets = Latency / SamplesPerPacket, InitialBlocks = 2 };

    CHECK((InitialBlocks - 1) * SourcePacketsInBlock * SamplesPerPacket < Latency);
    CHECK(InitialBlocks * SourcePacketsInBlock * SamplesPerPacket > Latency);

    init_with_defaults();

    if (!fec_supported()) {
        return;
    }

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* source_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioSource, source_proto, dst_addr1);
    packet::IWriter* repair_endpoint_writer = create_transport_endpoint(
        slot, address::Iface_AudioRepair, repair_proto, dst_addr2);

    test::FrameReader frame_reader(receiver, frame_factory);

    packet::FifoQueue source_queue;
    packet::FifoQueue repair_queue;

    test::PacketWriter packet_writer(arena, source_queue, repair_queue, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     dst_addr2, PayloadType_Ch2, fec_scheme, fec_config);

    packet_writer.write_packets(SourcePacketsInBlock * (InitialBlocks + 1),
                                SamplesPerPacket, packet_sample_spec);

    // initial latency
    size_t wr_packets = 0, rd_packets = 0;

    for (size_t n_blk = 0; n_blk < InitialBlocks; n_blk++) {
        for (size_t np = 0; np < SourcePacketsInBlock; np++) {
            packet::PacketPtr pp = read_packet(source_queue);
            CHECK(pp);
            write_packet(*source_endpoint_writer, pp);
            wr_packets++;

            if (wr_packets >= LatencyPackets) {
                refresh_source(receiver, frame_reader.refresh_ts());
                frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
                rd_packets++;
            }
        }

        for (size_t np = 0; np < RepairPacketsInBlock; np++) {
            packet::PacketPtr pp = read_packet(repair_queue);
            CHECK(pp);
            write_packet(*repair_endpoint_writer, pp);
        }
    }

    // read everything that we've accumulated
    while (rd_packets < wr_packets) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples(SamplesPerPacket, 1, output_sample_spec);
        rd_packets++;
    }

    LONGS_EQUAL(SourcePacketsInBlock, source_queue.size());
    LONGS_EQUAL(RepairPacketsInBlock, repair_queue.size());

    // get first 3 packets
    packet::PacketPtr p1 = read_packet(source_queue);
    packet::PacketPtr p2 = read_packet(source_queue);
    packet::PacketPtr p3 = read_packet(source_queue);

    // deliver P1
    write_packet(*source_endpoint_writer, p1);
    // deliver P3
    write_packet(*source_endpoint_writer, p3);

    // request 3 packets, get 1
    // (because P2 is lost and repair packets are delayed)
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples_soft(SamplesPerPacket * 3, SamplesPerPacket, 1,
                                   output_sample_spec);

    // request 1 packet, get 0
    refresh_source(receiver, frame_reader.refresh_ts());
    frame_reader.read_samples_soft(SamplesPerPacket, 0, 0, output_sample_spec);

    // deliver remaining source and repair packets, except P2
    // now P2 will be restored
    while (packet::PacketPtr pp = read_packet(source_queue)) {
        write_packet(*source_endpoint_writer, pp);
    }
    while (packet::PacketPtr pp = read_packet(repair_queue)) {
        write_packet(*repair_endpoint_writer, pp);
    }

    for (size_t np = 1; np < SourcePacketsInBlock; np++) {
        // request 1 packet, get 1
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_samples_soft(SamplesPerPacket, SamplesPerPacket, 1,
                                       output_sample_spec);
    }
}

// Read into big pre-allocated frame, larger than maximum size
// supported by frame buffer pool.
TEST(receiver_source, big_read) {
    enum {
        PacketsPerBigFrame = MaxBufSize / SamplesPerPacket * SamplesPerPacket * 2,
        SamplesPerBigFrame = SamplesPerPacket * PacketsPerBigFrame,
        NumFrames = 3,
    };

    CHECK(SamplesPerBigFrame > frame_factory.raw_buffer_size());

    init_with_defaults();

    ReceiverSource receiver(
        make_custom_config(Latency, LatencyTolerance * 100, Timeout * 100, Warmup),
        processor_map, encoding_map, packet_pool, packet_buffer_pool, frame_pool,
        frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    audio::FrameFactory big_frame_factory(
        arena,
        SamplesPerBigFrame * output_sample_spec.num_channels() * sizeof(audio::sample_t));

    audio::FramePtr big_frame = big_frame_factory.allocate_frame(
        output_sample_spec.stream_timestamp_2_bytes(SamplesPerBigFrame));
    CHECK(big_frame);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerPacket, output_sample_spec);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    for (size_t nf = 0; nf < NumFrames; nf++) {
        packet_writer.write_packets(PacketsPerBigFrame, SamplesPerPacket,
                                    packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        read_into_frame(receiver, *big_frame, output_sample_spec,
                        SamplesPerBigFrame * output_sample_spec.num_channels());

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

// Packets are stereo, receiver produces mono.
TEST(receiver_source, channel_mapping_stereo_to_mono) {
    enum { Rate = SampleRate, OutputChans = Chans_Mono, PacketChans = Chans_Stereo };

    const audio::PcmSubformat OutputFormat = Format_Raw;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, OutputChans, OutputFormat, Rate, PacketChans, PacketFormat);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Packets are mono, receiver produces stereo.
TEST(receiver_source, channel_mapping_mono_to_stereo) {
    enum { Rate = SampleRate, OutputChans = Chans_Stereo, PacketChans = Chans_Mono };

    const audio::PcmSubformat OutputFormat = Format_Raw;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, OutputChans, OutputFormat, Rate, PacketChans, PacketFormat);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// Packets have one rate, receiver produces different rate.
TEST(receiver_source, sample_rate_mapping) {
    enum { OutputRate = 48000, PacketRate = 44100, Chans = Chans_Stereo };

    const audio::PcmSubformat OutputFormat = Format_Raw;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(OutputRate, Chans, OutputFormat, PacketRate, Chans, PacketFormat);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_nonzero_samples(SamplesPerFrame * OutputRate / PacketRate
                                                  / output_sample_spec.num_channels()
                                                  * output_sample_spec.num_channels(),
                                              output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, format_mapping_s16) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    const audio::PcmSubformat OutputFormat = Format_S16_Ne;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, Chans, OutputFormat, Rate, Chans, PacketFormat);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_s16_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

TEST(receiver_source, format_mapping_s32) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    const audio::PcmSubformat OutputFormat = Format_S32_Ne;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, Chans, OutputFormat, Rate, Chans, PacketFormat);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_s32_samples(SamplesPerFrame, 1, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);
    }
}

// When there are no control packets, receiver always sets CTS of frames to zero.
TEST(receiver_source, timestamp_mapping_no_control_packets) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);

    packet::IWriter* transport_endpoint = create_transport_endpoint(
        slot, address::Iface_AudioSource, address::Proto_RTP, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                            dst_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // Expect no CTS.
    const core::nanoseconds_t capture_ts_base = -1;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts(capture_ts_base));
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
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);

    packet::IWriter* transport_endpoint = create_transport_endpoint(
        slot, address::Iface_AudioSource, address::Proto_RTP, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory, src_addr1,
                                       dst_addr2);

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

            refresh_source(receiver, frame_reader.refresh_ts(capture_ts_base));
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
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);

    packet::IWriter* transport_endpoint = create_transport_endpoint(
        slot, address::Iface_AudioSource, address::Proto_RTP, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory, src_addr1,
                                       dst_addr2);

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

            refresh_source(receiver, frame_reader.refresh_ts(capture_ts_base));
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

// Check CTS when there is also channel mapping, sample rate, and format mapping.
TEST(receiver_source, timestamp_mapping_remixing) {
    enum {
        OutputRate = 48000,
        PacketRate = 44100,
        OutputChans = Chans_Stereo,
        PacketChans = Chans_Mono
    };

    const audio::PcmSubformat OutputFormat = Format_S16_Ne;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(OutputRate, OutputChans, OutputFormat, PacketRate, PacketChans,
                    PacketFormat);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);

    packet::IWriter* transport_endpoint = create_transport_endpoint(
        slot, address::Iface_AudioSource, address::Proto_RTP, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch1);

    test::ControlWriter control_writer(*control_endpoint, packet_factory, src_addr1,
                                       dst_addr2);

    control_writer.set_local_source(src_id1);

    const core::nanoseconds_t unix_base = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    const size_t frame_size = SamplesPerFrame * OutputRate / PacketRate
        / output_sample_spec.num_channels() * output_sample_spec.num_channels();
    size_t frame_num = 0;
    core::nanoseconds_t first_ts = 0;

    core::nanoseconds_t cur_time = 2000000000000000;

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, cur_time);
            cur_time += output_sample_spec.samples_overall_2_ns(frame_size);

            audio::FramePtr frame = read_frame(receiver, output_sample_spec, frame_size);

            if (!first_ts && frame->capture_timestamp()) {
                first_ts = frame->capture_timestamp();

                CHECK(first_ts >= unix_base);
                CHECK(first_ts < unix_base + core::Second);
            }

            if (first_ts) {
                const core::nanoseconds_t expected_capture_ts = first_ts
                    + output_sample_spec.samples_overall_2_ns(frame_num * frame_size);

                test::expect_capture_timestamp(
                    expected_capture_ts, frame->capture_timestamp(), output_sample_spec,
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

// Set high jitter, wait until latency increases and stabilizes.
TEST(receiver_source, adaptive_latency_increase) {
    const size_t stabilization_window = JitterMeterWindow * 5;

    const core::nanoseconds_t tolerance = core::Millisecond * 5;
    const core::nanoseconds_t reaction = core::Second;

    const core::nanoseconds_t min_target_latency = core::Millisecond * 10;
    const core::nanoseconds_t max_target_latency = core::Millisecond * 500;

    const core::nanoseconds_t start_latency = core::Millisecond * 50;
    const core::nanoseconds_t jitter = core::Millisecond * 30;

    const core::nanoseconds_t expected_min = jitter * 3;
    const core::nanoseconds_t expected_max = jitter * 6;

    init_with_defaults();

    ReceiverSource receiver(make_adaptive_config(start_latency, min_target_latency,
                                                 max_target_latency, tolerance, reaction),
                            processor_map, encoding_map, packet_pool, packet_buffer_pool,
                            frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // set jitter higher than start latency
    packet_writer.set_jitter(jitter - tolerance, jitter + tolerance);

    // wait until we reach stable latency
    stat::MovAggregate<core::nanoseconds_t> latency_hist(arena, stabilization_window);
    CHECK(latency_hist.is_valid());

    for (;;) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        if (cur_latency > 0) {
            latency_hist.add(cur_latency);
        }
        if (latency_hist.is_full() && latency_hist.mov_min() > expected_min
            && latency_hist.mov_max() < expected_max
            && std::abs(latency_hist.mov_max() - latency_hist.mov_min()) < tolerance) {
            break;
        }
    }

    const core::nanoseconds_t stable_latency = latency_hist.mov_max();

    roc_log(LogNote, "reached stable latency: %.3fms",
            (double)stable_latency / core::Millisecond);

    CHECK(stable_latency > expected_min);
    CHECK(stable_latency < expected_max);

    // ensure we've stabilized
    for (size_t np = 0; np < stabilization_window; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_nonzero_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        CHECK(std::abs(cur_latency - stable_latency) < tolerance);
    }
}

// Set low jitter, wait until latency decreases and stabilizes.
TEST(receiver_source, adaptive_latency_decrease) {
    const size_t stabilization_window = JitterMeterWindow * 5;

    const core::nanoseconds_t tolerance = core::Millisecond * 5;
    const core::nanoseconds_t reaction = core::Second;

    const core::nanoseconds_t min_target_latency = core::Millisecond * 10;
    const core::nanoseconds_t max_target_latency = core::Millisecond * 500;

    const core::nanoseconds_t start_latency = core::Millisecond * 120;
    const core::nanoseconds_t jitter = core::Millisecond * 20;

    const core::nanoseconds_t expected_min = jitter * 3;
    const core::nanoseconds_t expected_max = jitter * 6;

    init_with_defaults();

    ReceiverSource receiver(make_adaptive_config(start_latency, min_target_latency,
                                                 max_target_latency, tolerance, reaction),
                            processor_map, encoding_map, packet_pool, packet_buffer_pool,
                            frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // set jitter lower than start latency
    packet_writer.set_jitter(jitter - tolerance, jitter + tolerance);

    // wait until we reach stable latency
    stat::MovAggregate<core::nanoseconds_t> latency_hist(arena, stabilization_window);
    CHECK(latency_hist.is_valid());

    for (;;) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        if (cur_latency > 0) {
            latency_hist.add(cur_latency);
        }
        if (latency_hist.is_full() && latency_hist.mov_min() > expected_min
            && latency_hist.mov_max() < expected_max
            && std::abs(latency_hist.mov_max() - latency_hist.mov_min()) < tolerance) {
            break;
        }
    }

    const core::nanoseconds_t stable_latency = latency_hist.mov_min();

    roc_log(LogNote, "reached stable latency: %.3fms",
            (double)stable_latency / core::Millisecond);

    CHECK(stable_latency > expected_min);
    CHECK(stable_latency < expected_max);

    // ensure we've stabilized
    for (size_t np = 0; np < stabilization_window; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_nonzero_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        CHECK(std::abs(cur_latency - stable_latency) < tolerance);
    }
}

// Adaptive latency should be bounded by max_target_latency
TEST(receiver_source, adaptive_latency_upper_bound) {
    const size_t stabilization_window = JitterMeterWindow * 5;

    const core::nanoseconds_t tolerance = core::Millisecond * 5;
    const core::nanoseconds_t reaction = core::Second;

    const core::nanoseconds_t min_target_latency = core::Millisecond * 100;
    const core::nanoseconds_t max_target_latency = core::Millisecond * 140;
    const core::nanoseconds_t start_latency = core::Millisecond * 120;

    init_with_defaults();

    ReceiverSource receiver(make_adaptive_config(start_latency, min_target_latency,
                                                 max_target_latency, tolerance, reaction),
                            processor_map, encoding_map, packet_pool, packet_buffer_pool,
                            frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // set jitter higher than max latency
    packet_writer.set_jitter(max_target_latency * 2 - tolerance,
                             max_target_latency * 2 + tolerance);

    // wait until we reach maximum latency
    for (;;) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        if (cur_latency >= max_target_latency - tolerance / 2) {
            break;
        }
    }

    // ensure we've stabilized
    for (size_t np = 0; np < stabilization_window; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_nonzero_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        CHECK(std::abs(cur_latency - max_target_latency) < tolerance);
    }
}

// Adaptive latency should be bounded by min_target_latency
TEST(receiver_source, adaptive_latency_lower_bound) {
    const size_t stabilization_window = JitterMeterWindow * 5;

    const core::nanoseconds_t tolerance = core::Millisecond * 5;
    const core::nanoseconds_t reaction = core::Second;

    const core::nanoseconds_t min_target_latency = core::Millisecond * 100;
    const core::nanoseconds_t max_target_latency = core::Millisecond * 140;
    const core::nanoseconds_t start_latency = core::Millisecond * 120;

    init_with_defaults();

    ReceiverSource receiver(make_adaptive_config(start_latency, min_target_latency,
                                                 max_target_latency, tolerance, reaction),
                            processor_map, encoding_map, packet_pool, packet_buffer_pool,
                            frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // set jitter higher than max latency
    packet_writer.set_jitter(min_target_latency / 10 - tolerance,
                             min_target_latency / 10 + tolerance);

    // wait until we reach minimum latency
    for (;;) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        if (cur_latency > 0 && cur_latency <= min_target_latency + tolerance / 2) {
            break;
        }
    }

    // ensure we've stabilized
    for (size_t np = 0; np < stabilization_window; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_nonzero_samples(SamplesPerPacket, output_sample_spec);

        const core::nanoseconds_t cur_latency = get_niq_latency(*slot);
        CHECK(std::abs(cur_latency - min_target_latency) < tolerance);
    }
}

// Check receiver metrics for multiple remote participants (senders).
TEST(receiver_source, metrics_participants) {
    enum { MaxParties = 10 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

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

    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

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
            refresh_source(receiver, frame_reader.refresh_ts());
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

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id2, src_addr2, dst_addr1,
                                      PayloadType_Ch2);

    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
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
    enum { MaxParties = 10 };

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *endpoint_writer, encoding_map,
                                      packet_factory, src_id2, src_addr2, dst_addr1,
                                      PayloadType_Ch2);

    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        refresh_source(receiver, frame_reader.refresh_ts());
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
// expected_packets, lost_packets, ext_first_seqnum, ext_last_seqnum
TEST(receiver_source, metrics_packet_counters) {
    enum { InitSeqnum = 0xFFFC };
    uint32_t seqnum = InitSeqnum;
    uint32_t prev_seqnum = InitSeqnum;
    size_t pkt_counter = 0;
    size_t prev_pkt_counter = 0;
    size_t pkt_lost_counter = 0;
    size_t prev_pkt_lost_counter = 0;

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    {
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics;

        slot->get_metrics(slot_metrics, &party_metrics, NULL);
    }

    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);
    packet_writer.set_seqnum(InitSeqnum);
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                output_sample_spec);
    pkt_counter += Latency / SamplesPerPacket;
    prev_pkt_counter = pkt_counter;
    prev_seqnum = seqnum = InitSeqnum + pkt_counter - 1;

    {
        ReceiverSlotMetrics slot_metrics;
        ReceiverParticipantMetrics party_metrics;
        size_t party_metrics_size = 1;

        slot->get_metrics(slot_metrics, &party_metrics, &party_metrics_size);

        CHECK(slot_metrics.source_id != 0);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        const bool lose_pkt = np % 3 == 0 && np;
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_any_samples(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        if (lose_pkt) {
            packet_writer.skip_packets(1, SamplesPerPacket, output_sample_spec);
        } else {
            packet_writer.write_packets(1, SamplesPerPacket, output_sample_spec);
        }

        {
            ReceiverSlotMetrics slot_metrics;
            ReceiverParticipantMetrics party_metrics;
            size_t party_metrics_size = 1;

            slot->get_metrics(slot_metrics, &party_metrics, &party_metrics_size);

            if (!lose_pkt) {
                UNSIGNED_LONGS_EQUAL(prev_pkt_counter,
                                     party_metrics.link.expected_packets);
                UNSIGNED_LONGS_EQUAL(InitSeqnum, party_metrics.link.ext_first_seqnum);
                UNSIGNED_LONGS_EQUAL(prev_pkt_lost_counter,
                                     party_metrics.link.lost_packets);
                UNSIGNED_LONGS_EQUAL(prev_seqnum, party_metrics.link.ext_last_seqnum);
            }
        }

        prev_pkt_lost_counter = pkt_lost_counter;
        if (lose_pkt) {
            pkt_lost_counter++;
        }
        pkt_counter++;
        seqnum++;
        if (!lose_pkt) {
            prev_pkt_counter = pkt_counter;
            prev_seqnum = seqnum;
        }
    }
}

// Check how receiver computes jitter metric.
TEST(receiver_source, metrics_jitter) {
    const core::nanoseconds_t jitter1 = core::Millisecond * 40;
    const core::nanoseconds_t jitter2 = core::Millisecond * 80;
    const core::nanoseconds_t precision = core::Millisecond * 5;

    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    // jitter 1
    packet_writer.set_jitter(jitter1 - precision, jitter1 + precision);

    for (size_t np = 0; np < JitterMeterWindow * 2; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerPacket, output_sample_spec);

        {
            ReceiverSlotMetrics slot_metrics;
            ReceiverParticipantMetrics party_metrics;
            size_t party_metrics_size = 1;

            slot->get_metrics(slot_metrics, &party_metrics, &party_metrics_size);

            CHECK(slot_metrics.source_id != 0);
            UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

            if (np > Latency / SamplesPerPacket) {
                DOUBLES_EQUAL(jitter1, party_metrics.link.peak_jitter, precision);
            }
        }
    }

    // jitter 2
    packet_writer.set_jitter(jitter2 - precision, jitter2 + precision);

    for (size_t np = 0; np < JitterMeterWindow * 2; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, packet_sample_spec);

        refresh_source(receiver, frame_reader.refresh_ts());
        frame_reader.read_any_samples(SamplesPerPacket, output_sample_spec);

        {
            ReceiverSlotMetrics slot_metrics;
            ReceiverParticipantMetrics party_metrics;
            size_t party_metrics_size = 1;

            slot->get_metrics(slot_metrics, &party_metrics, &party_metrics_size);

            CHECK(slot_metrics.source_id != 0);
            UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

            if (np > JitterMeterWindow) {
                DOUBLES_EQUAL(jitter2, party_metrics.link.peak_jitter, precision);
            }
        }
    }
}

// Check how receiver computes niq_latency metric (network incoming queue size).
TEST(receiver_source, metrics_niq_latency) {
    enum { MaxParties = 10 };

    init_with_defaults();

    const core::nanoseconds_t virtual_niq_latency =
        output_sample_spec.samples_per_chan_2_ns(Latency);

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                output_sample_spec);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
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
    enum { MaxParties = 10 };

    init_with_defaults();

    const core::nanoseconds_t virtual_e2e_latency = core::Millisecond * 555;

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory, src_addr1,
                                       dst_addr2);

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

            refresh_source(receiver, frame_reader.refresh_ts(capture_ts_base));
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

// Check that no reports are generated by receiver when there are no senders.
TEST(receiver_source, reports_no_senders) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                            dst_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    for (size_t np = 0; np < (ReportInterval / SamplesPerPacket) * ManyReports; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            refresh_source(receiver, frame_reader.refresh_ts());
            frame_reader.read_zero_samples(SamplesPerFrame, output_sample_spec);

            UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
        }

        UNSIGNED_LONGS_EQUAL(0, control_outbound_queue.size());
    }
}

// Check reports generated by receiver when there is one sender.
TEST(receiver_source, reports_one_sender) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);

    packet::stream_source_t recv_src_id = 0;
    packet::stream_source_t send_src_id = 0;

    {
        ReceiverSlotMetrics slot_metrics;
        slot->get_metrics(slot_metrics, NULL, NULL);
        recv_src_id = slot_metrics.source_id;
        send_src_id = slot_metrics.source_id + 9999;
    }

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer(arena, *transport_endpoint, encoding_map,
                                     packet_factory, send_src_id, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory, src_addr1,
                                       dst_addr2);

    control_writer.set_local_source(send_src_id);

    test::ControlReader control_reader(control_outbound_queue);

    const core::nanoseconds_t capture_ts_base = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer.set_timestamp(rtp_base);
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                output_sample_spec);

    size_t next_report = ReportInterval / SamplesPerPacket;

    for (size_t np = 0; np < (ReportInterval / SamplesPerPacket) * ManyReports; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            core::nanoseconds_t expect_ts_base = -1;
            if (np != 0) {
                expect_ts_base = capture_ts_base;
            }

            refresh_source(receiver, frame_reader.refresh_ts(capture_ts_base));
            frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec,
                                              expect_ts_base);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        if (np > next_report) {
            control_reader.read_report();

            CHECK(!control_reader.has_src_addr());
            CHECK(control_reader.has_dst_addr(src_addr1));
            CHECK(!control_reader.has_sr());
            CHECK(control_reader.has_rr(recv_src_id, send_src_id));
            CHECK(control_reader.has_rrtr(recv_src_id));
            CHECK(!control_reader.has_dlrr());
            CHECK(control_reader.has_measurement_info(recv_src_id));
            CHECK(control_reader.has_delay_metrics(recv_src_id));
            CHECK(control_reader.has_queue_metrics(recv_src_id));

            next_report = np + ReportInterval / SamplesPerPacket;
        }

        packet_writer.write_packets(1, SamplesPerPacket, output_sample_spec);

        if (np % (ReportInterval / SamplesPerPacket) == 0) {
            control_writer.write_sender_report(packet::unix_2_ntp(capture_ts_base),
                                               rtp_base);
        }
    }
}

// Check reports generated by receiver when there are two unicast senders.
// Receiver should generate separate report for each sender.
TEST(receiver_source, reports_two_senders_unicast) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    CHECK(slot);

    packet::stream_source_t recv_src_id = 0;
    packet::stream_source_t send_src_id1 = 0;
    packet::stream_source_t send_src_id2 = 0;

    {
        ReceiverSlotMetrics slot_metrics;
        slot->get_metrics(slot_metrics, NULL, NULL);
        recv_src_id = slot_metrics.source_id;
        send_src_id1 = slot_metrics.source_id + 7777;
        send_src_id2 = slot_metrics.source_id + 9999;
    }

    packet::IWriter* transport_endpoint =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *transport_endpoint, encoding_map,
                                      packet_factory, send_src_id1, src_addr1, dst_addr1,
                                      PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *transport_endpoint, encoding_map,
                                      packet_factory, send_src_id2, src_addr2, dst_addr1,
                                      PayloadType_Ch2);

    test::ControlWriter control_writer1(*control_endpoint, packet_factory, src_addr1,
                                        dst_addr2);

    test::ControlWriter control_writer2(*control_endpoint, packet_factory, src_addr2,
                                        dst_addr2);

    control_writer1.set_cname("test_cname1");
    control_writer2.set_cname("test_cname2");

    control_writer1.set_local_source(send_src_id1);
    control_writer2.set_local_source(send_src_id2);

    test::ControlReader control_reader(control_outbound_queue);

    const core::nanoseconds_t capture_ts_base = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer1.set_timestamp(rtp_base);
    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    packet_writer2.set_timestamp(rtp_base);
    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    size_t next_report = ReportInterval / SamplesPerPacket;

    for (size_t np = 0; np < (ReportInterval / SamplesPerPacket) * ManyReports; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            core::nanoseconds_t expect_ts_base = -1;
            if (np != 0) {
                expect_ts_base = capture_ts_base;
            }

            refresh_source(receiver, frame_reader.refresh_ts(capture_ts_base));
            frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec,
                                              expect_ts_base);
        }

        if (np > next_report) {
            size_t n_sess1 = 0, n_sess2 = 0;

            for (size_t nr = 0; nr < 2; nr++) {
                control_reader.read_report();

                if (control_reader.dst_addr() == src_addr1) {
                    CHECK(!control_reader.has_src_addr());
                    CHECK(control_reader.has_dst_addr(src_addr1));
                    CHECK(!control_reader.has_sr());
                    CHECK(control_reader.has_rr(recv_src_id, send_src_id1));
                    CHECK(control_reader.has_rrtr(recv_src_id));
                    CHECK(!control_reader.has_dlrr());
                    CHECK(control_reader.has_measurement_info(recv_src_id));
                    CHECK(control_reader.has_delay_metrics(recv_src_id));
                    CHECK(control_reader.has_queue_metrics(recv_src_id));
                    n_sess1++;
                } else {
                    CHECK(!control_reader.has_src_addr());
                    CHECK(control_reader.has_dst_addr(src_addr2));
                    CHECK(!control_reader.has_sr());
                    CHECK(control_reader.has_rr(recv_src_id, send_src_id2));
                    CHECK(control_reader.has_rrtr(recv_src_id));
                    CHECK(!control_reader.has_dlrr());
                    CHECK(control_reader.has_measurement_info(recv_src_id));
                    CHECK(control_reader.has_delay_metrics(recv_src_id));
                    CHECK(control_reader.has_queue_metrics(recv_src_id));
                    n_sess2++;
                }
            }

            LONGS_EQUAL(1, n_sess1);
            LONGS_EQUAL(1, n_sess2);

            next_report = np + ReportInterval / SamplesPerPacket;
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);

        if (np % (ReportInterval / SamplesPerPacket) == 0) {
            control_writer1.write_sender_report(packet::unix_2_ntp(capture_ts_base),
                                                rtp_base);
            control_writer2.write_sender_report(packet::unix_2_ntp(capture_ts_base),
                                                rtp_base);
        }
    }
}

// Check reports generated by receiver when there are two senders in multicast session.
// Receiver should generate single combined report for all senders.
TEST(receiver_source, reports_two_senders_multicast) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);

    packet::stream_source_t recv_src_id = 0;
    packet::stream_source_t send_src_id1 = 0;
    packet::stream_source_t send_src_id2 = 0;

    {
        ReceiverSlotMetrics slot_metrics;
        slot->get_metrics(slot_metrics, NULL, NULL);
        recv_src_id = slot_metrics.source_id;
        send_src_id1 = slot_metrics.source_id + 7777;
        send_src_id2 = slot_metrics.source_id + 9999;
    }

    packet::IWriter* transport_endpoint = create_transport_endpoint(
        slot, address::Iface_AudioSource, proto1, multicast_addr1);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                multicast_addr2, control_outbound_queue);

    test::FrameReader frame_reader(receiver, frame_factory);

    test::PacketWriter packet_writer1(arena, *transport_endpoint, encoding_map,
                                      packet_factory, send_src_id1, src_addr1,
                                      multicast_addr1, PayloadType_Ch2);

    test::PacketWriter packet_writer2(arena, *transport_endpoint, encoding_map,
                                      packet_factory, send_src_id2, src_addr2,
                                      multicast_addr1, PayloadType_Ch2);

    test::ControlWriter control_writer1(*control_endpoint, packet_factory, src_addr1,
                                        multicast_addr2);

    test::ControlWriter control_writer2(*control_endpoint, packet_factory, src_addr2,
                                        multicast_addr2);

    control_writer1.set_cname("test_cname1");
    control_writer2.set_cname("test_cname2");

    control_writer1.set_local_source(send_src_id1);
    control_writer2.set_local_source(send_src_id2);

    test::ControlReader control_reader(control_outbound_queue);

    const core::nanoseconds_t capture_ts_base = 1000000000000000;
    const packet::stream_timestamp_t rtp_base = 1000000;

    packet_writer1.set_timestamp(rtp_base);
    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    packet_writer2.set_timestamp(rtp_base);
    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                 output_sample_spec);

    size_t next_report = ReportInterval / SamplesPerPacket;

    for (size_t np = 0; np < (ReportInterval / SamplesPerPacket) * ManyReports; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            core::nanoseconds_t expect_ts_base = -1;
            if (np != 0) {
                expect_ts_base = capture_ts_base;
            }

            refresh_source(receiver, frame_reader.refresh_ts(capture_ts_base));
            frame_reader.read_nonzero_samples(SamplesPerFrame, output_sample_spec,
                                              expect_ts_base);
        }

        if (np > next_report) {
            control_reader.read_report();

            CHECK(!control_reader.has_src_addr());
            CHECK(control_reader.has_dst_addr(multicast_addr2));
            CHECK(!control_reader.has_sr());
            CHECK(control_reader.has_rr(recv_src_id, send_src_id1));
            CHECK(control_reader.has_rr(recv_src_id, send_src_id2));
            CHECK(control_reader.has_rrtr(recv_src_id));
            CHECK(!control_reader.has_dlrr());
            CHECK(control_reader.has_measurement_info(recv_src_id));
            CHECK(control_reader.has_delay_metrics(recv_src_id));
            CHECK(control_reader.has_queue_metrics(recv_src_id));

            next_report = np + ReportInterval / SamplesPerPacket;
        }

        packet_writer1.write_packets(1, SamplesPerPacket, output_sample_spec);
        packet_writer2.write_packets(1, SamplesPerPacket, output_sample_spec);

        if (np % (ReportInterval / SamplesPerPacket) == 0) {
            control_writer1.write_sender_report(packet::unix_2_ntp(capture_ts_base),
                                                rtp_base);
            control_writer2.write_sender_report(packet::unix_2_ntp(capture_ts_base),
                                                rtp_base);
        }
    }
}

TEST(receiver_source, pipeline_state) {
    init_with_defaults();

    ReceiverSource receiver(make_default_config(), processor_map, encoding_map,
                            packet_pool, packet_buffer_pool, frame_pool,
                            frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverSlot* slot = create_slot(receiver);
    packet::IWriter* endpoint_writer =
        create_transport_endpoint(slot, address::Iface_AudioSource, proto1, dst_addr1);

    test::PacketWriter packet_writer(arena, *endpoint_writer, encoding_map,
                                     packet_factory, src_id1, src_addr1, dst_addr1,
                                     PayloadType_Ch2);

    const size_t frame_size = FramesPerPacket * output_sample_spec.num_channels();

    core::nanoseconds_t cur_time = 1000000000000000;

    CHECK(receiver.state() == sndio::DeviceState_Idle);

    {
        refresh_source(receiver, cur_time);
        cur_time += output_sample_spec.samples_overall_2_ns(frame_size);

        read_frame(receiver, output_sample_spec, frame_size);
    }

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket,
                                packet_sample_spec);

    CHECK(receiver.state() == sndio::DeviceState_Active);

    {
        refresh_source(receiver, cur_time);
        cur_time += output_sample_spec.samples_overall_2_ns(frame_size);

        read_frame(receiver, output_sample_spec, frame_size);
    }

    for (;;) {
        refresh_source(receiver, cur_time);
        cur_time += output_sample_spec.samples_overall_2_ns(frame_size);

        read_frame(receiver, output_sample_spec, frame_size);

        if (receiver.state() == sndio::DeviceState_Idle) {
            break;
        }
    }
}

} // namespace pipeline
} // namespace roc
