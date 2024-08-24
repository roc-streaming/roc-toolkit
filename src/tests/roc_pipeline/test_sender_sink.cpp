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
#include "test_helpers/frame_writer.h"
#include "test_helpers/packet_reader.h"

#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_core/time.h"
#include "roc_packet/fifo_queue.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/encoding_map.h"

// This file contains tests for SenderSink. SenderSink can be seen as a big
// composite processor (consisting of chained smaller processors) that transforms
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

const audio::PcmSubformat Format_Raw = audio::PcmSubformat_Raw;
const audio::PcmSubformat Format_S16_Be = audio::PcmSubformat_SInt16_Be;
const audio::PcmSubformat Format_S16_Ne = audio::PcmSubformat_SInt16;
const audio::PcmSubformat Format_S32_Ne = audio::PcmSubformat_SInt32;

const rtp::PayloadType PayloadType_Ch1 = rtp::PayloadType_L16_Mono;
const rtp::PayloadType PayloadType_Ch2 = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 1000,

    SampleRate = 44100,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    ReportInterval = SamplesPerPacket * 10,
    ReportTimeout = SamplesPerPacket * 100,

    ManyFrames = FramesPerPacket * 20,
    ManyReports = 20,
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

SenderSlot* create_slot(SenderSink& sink) {
    SenderSlotConfig slot_config;
    SenderSlot* slot = sink.create_slot(slot_config);
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
    packet::IWriter* writer = endpoint->inbound_writer();
    CHECK(writer);
    return writer;
}

void refresh_sink(SenderSink& sender_sink, core::nanoseconds_t refresh_ts) {
    LONGS_EQUAL(status::StatusOK, sender_sink.refresh(refresh_ts, NULL));
}

} // namespace

TEST_GROUP(sender_sink) {
    audio::SampleSpec input_sample_spec;
    audio::SampleSpec packet_sample_spec;

    address::Protocol proto;

    address::SocketAddr src_addr1;
    address::SocketAddr src_addr2;

    address::SocketAddr dst_addr1;
    address::SocketAddr dst_addr2;

    SenderSinkConfig make_config() {
        SenderSinkConfig config;

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
        config.enable_cpu_clock = false;
        config.enable_profiling = true;

        config.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        config.latency.tuner_profile = audio::LatencyTunerProfile_Intact;

        config.rtcp.report_interval = ReportInterval * core::Second / SampleRate;
        config.rtcp.inactivity_timeout = ReportTimeout * core::Second / SampleRate;

        return config;
    }

    void init_with_specs(int input_sample_rate, audio::ChannelMask input_channels,
                         audio::PcmSubformat input_format, int packet_sample_rate,
                         audio::ChannelMask packet_channels,
                         audio::PcmSubformat packet_format) {
        input_sample_spec.set_format(audio::Format_Pcm);
        input_sample_spec.set_pcm_subformat(input_format);
        input_sample_spec.set_sample_rate((size_t)input_sample_rate);
        input_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        input_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        input_sample_spec.channel_set().set_mask(input_channels);

        packet_sample_spec.set_format(audio::Format_Pcm);
        packet_sample_spec.set_pcm_subformat(packet_format);
        packet_sample_spec.set_sample_rate((size_t)packet_sample_rate);
        packet_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        packet_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        packet_sample_spec.channel_set().set_mask(packet_channels);

        proto = address::Proto_RTP;

        src_addr1 = test::new_address(11);
        src_addr2 = test::new_address(12);

        dst_addr1 = test::new_address(21);
        dst_addr2 = test::new_address(22);
    }

    void init_with_defaults() {
        init_with_specs(SampleRate, Chans_Stereo, Format_Raw, SampleRate, Chans_Stereo,
                        Format_S16_Be);
    }
};

TEST(sender_sink, basic) {
    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames smaller than packets.
TEST(sender_sink, frame_size_small) {
    enum {
        SamplesPerSmallFrame = SamplesPerFrame / 2,
        SmallFramesPerPacket = SamplesPerPacket / SamplesPerSmallFrame,
        ManySmallFrames = SmallFramesPerPacket * 20
    };

    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame, input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames larger than packets.
TEST(sender_sink, frame_size_large) {
    enum {
        SamplesPerLargeFrame = SamplesPerPacket * 4,
        PacketsPerLargeFrame = SamplesPerLargeFrame / SamplesPerPacket,
        ManyLargeFrames = 20
    };

    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame, input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames written to sender are stereo, packets are mono.
TEST(sender_sink, channel_mapping_stereo_to_mono) {
    enum { Rate = SampleRate, InputChans = Chans_Stereo, PacketChans = Chans_Mono };

    const audio::PcmSubformat InputFormat = Format_Raw;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, InputChans, InputFormat, Rate, PacketChans, PacketFormat);

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch1);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Frames written to sender are mono, packets are stereo.
TEST(sender_sink, channel_mapping_mono_to_stereo) {
    enum { Rate = SampleRate, InputChans = Chans_Mono, PacketChans = Chans_Stereo };

    const audio::PcmSubformat InputFormat = Format_Raw;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, InputChans, InputFormat, Rate, PacketChans, PacketFormat);

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Different sample rate of frames and packets.
TEST(sender_sink, sample_rate_mapping) {
    enum { InputRate = 48000, PacketRate = 44100, Chans = Chans_Stereo };

    const audio::PcmSubformat InputFormat = Format_Raw;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(InputRate, Chans, InputFormat, PacketRate, Chans, PacketFormat);

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * InputRate / PacketRate
                                       / input_sample_spec.num_channels()
                                       * input_sample_spec.num_channels(),
                                   input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket - 5; np++) {
        packet_reader.read_nonzero_packet(SamplesPerPacket, packet_sample_spec);
    }
}

TEST(sender_sink, format_mapping_s16) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    const audio::PcmSubformat InputFormat = Format_S16_Ne;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, Chans, InputFormat, Rate, Chans, PacketFormat);

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_s16_samples(SamplesPerFrame, input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

TEST(sender_sink, format_mapping_s32) {
    enum { Rate = SampleRate, Chans = Chans_Stereo };

    const audio::PcmSubformat InputFormat = Format_S32_Ne;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(Rate, Chans, InputFormat, Rate, Chans, PacketFormat);

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_s32_samples(SamplesPerFrame, input_sample_spec);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec);
    }

    packet_reader.read_eof();
}

// Check how sender sets CTS of packets based on CTS of frames written to it.
TEST(sender_sink, timestamp_mapping) {
    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);
    }

    packet_reader.read_eof();
}

// Same as above, but there is also channel mapping, sample rate, and format mapping.
TEST(sender_sink, timestamp_mapping_remixing) {
    enum {
        InputRate = 48000,
        PacketRate = 44100,
        InputChans = Chans_Stereo,
        PacketChans = Chans_Mono
    };

    const audio::PcmSubformat InputFormat = Format_S16_Ne;
    const audio::PcmSubformat PacketFormat = Format_S16_Be;

    init_with_specs(InputRate, InputChans, InputFormat, PacketRate, PacketChans,
                    PacketFormat);

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_s16_samples(SamplesPerFrame * InputRate / PacketRate
                                           / input_sample_spec.num_channels()
                                           * input_sample_spec.num_channels(),
                                       input_sample_spec, unix_base);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch1);

    core::nanoseconds_t cts = 0;
    for (size_t np = 0; np < ManyFrames / FramesPerPacket - 5; np++) {
        packet::PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, queue.read(pp, packet::ModeFetch));
        CHECK(pp);

        if (np == 0) {
            cts = pp->rtp()->capture_timestamp;
            CHECK(cts >= unix_base);
            CHECK(cts < unix_base + core::Second);
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
    enum { MaxParties = 10 };

    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);
    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    const core::nanoseconds_t unix_base = 1000000000000000;

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
        refresh_sink(sender, frame_writer.refresh_ts());
    }

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);
    }

    CHECK(control_outbound_queue.size() > 0);

    packet::stream_source_t send_src_id = 0;
    packet::stream_source_t recv_src_id = 0;

    {
        SenderSlotMetrics slot_metrics;
        SenderParticipantMetrics party_metrics[MaxParties];
        size_t party_metrics_size = MaxParties;

        slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

        CHECK(slot_metrics.source_id != 0);

        send_src_id = slot_metrics.source_id;
        recv_src_id = slot_metrics.source_id + 9999;

        UNSIGNED_LONGS_EQUAL(0, slot_metrics.num_participants);
        UNSIGNED_LONGS_EQUAL(0, party_metrics_size);
    }

    test::ControlWriter control_writer(*control_endpoint, packet_factory, dst_addr1,
                                       src_addr1);

    control_writer.set_local_source(recv_src_id);
    control_writer.set_remote_source(send_src_id);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        const unsigned seed = (unsigned)np + 1;

        packet::LinkMetrics link_metrics;
        link_metrics.ext_first_seqnum = seed * 100;
        link_metrics.ext_last_seqnum = seed * 200;
        link_metrics.expected_packets = (seed * 200) - (seed * 100) + 1;
        link_metrics.lost_packets = (int)seed * 40;
        link_metrics.peak_jitter = (int)seed * core::Millisecond * 50;

        audio::LatencyMetrics latency_metrics;
        latency_metrics.niq_latency = (int)seed * core::Millisecond * 50;
        latency_metrics.niq_stalling = (int)seed * core::Millisecond * 60;
        latency_metrics.e2e_latency = (int)seed * core::Millisecond * 70;

        control_writer.set_link_metrics(link_metrics);
        control_writer.set_latency_metrics(latency_metrics);

        control_writer.write_receiver_report(
            packet::unix_2_ntp(frame_writer.refresh_ts()), packet_sample_spec);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
            refresh_sink(sender, frame_writer.refresh_ts());
        }
        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);

        {
            SenderSlotMetrics slot_metrics;
            SenderParticipantMetrics party_metrics[MaxParties];
            size_t party_metrics_size = MaxParties;

            slot->get_metrics(slot_metrics, party_metrics, &party_metrics_size);

            UNSIGNED_LONGS_EQUAL(send_src_id, slot_metrics.source_id);
            UNSIGNED_LONGS_EQUAL(1, slot_metrics.num_participants);
            UNSIGNED_LONGS_EQUAL(1, party_metrics_size);

            UNSIGNED_LONGS_EQUAL(link_metrics.ext_first_seqnum,
                                 party_metrics[0].link.ext_first_seqnum);
            UNSIGNED_LONGS_EQUAL(link_metrics.ext_last_seqnum,
                                 party_metrics[0].link.ext_last_seqnum);
            UNSIGNED_LONGS_EQUAL(link_metrics.expected_packets,
                                 party_metrics[0].link.expected_packets);
            UNSIGNED_LONGS_EQUAL(link_metrics.lost_packets,
                                 party_metrics[0].link.lost_packets);
            DOUBLES_EQUAL((double)link_metrics.peak_jitter,
                          (double)party_metrics[0].link.peak_jitter, core::Nanosecond);

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

// Check reports generated by sender when there are no discovered receivers.
// Generated reports should not have blocks dedicated for specific receivers.
TEST(sender_sink, reports_no_receivers) {
    enum { MaxParties = 10 };

    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);

    packet::stream_source_t send_src_id = 0;

    {
        SenderSlotMetrics slot_metrics;
        slot->get_metrics(slot_metrics, NULL, NULL);
        send_src_id = slot_metrics.source_id;
    }

    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    packet::FifoQueue control_outbound_queue;
    create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                            dst_addr2, control_outbound_queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    test::ControlReader control_reader(control_outbound_queue);

    const core::nanoseconds_t unix_base = 1000000000000000;

    size_t next_report = ReportInterval / SamplesPerPacket;

    for (size_t np = 0; np < (ReportInterval / SamplesPerPacket) * ManyReports; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
            refresh_sink(sender, frame_writer.refresh_ts());
        }

        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);

        if (np > next_report) {
            control_reader.read_report();

            CHECK(!control_reader.has_src_addr());
            CHECK(control_reader.has_dst_addr(dst_addr2));
            CHECK(control_reader.has_sr(send_src_id));
            CHECK(!control_reader.has_rr());
            CHECK(!control_reader.has_rrtr());
            CHECK(!control_reader.has_dlrr());
            CHECK(!control_reader.has_measurement_info());
            CHECK(!control_reader.has_delay_metrics());
            CHECK(!control_reader.has_queue_metrics());

            next_report = np + ReportInterval / SamplesPerPacket;
        }
    }
}

// Check reports generated by sender when there is one discovered receiver.
// Generated reports should have blocks dedicated for receiver.
TEST(sender_sink, reports_one_receiver) {
    enum { MaxParties = 10 };

    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);

    packet::stream_source_t send_src_id = 0;
    packet::stream_source_t recv_src_id = 0;

    {
        SenderSlotMetrics slot_metrics;
        slot->get_metrics(slot_metrics, NULL, NULL);
        send_src_id = slot_metrics.source_id;
        recv_src_id = slot_metrics.source_id + 9999;
    }

    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    test::ControlWriter control_writer(*control_endpoint, packet_factory, dst_addr2,
                                       src_addr1);

    control_writer.set_local_source(recv_src_id);
    control_writer.set_remote_source(send_src_id);

    test::ControlReader control_reader(control_outbound_queue);

    const core::nanoseconds_t unix_base = 1000000000000000;

    size_t next_report = ReportInterval / SamplesPerPacket;
    size_t n_reports = 0;

    for (size_t np = 0; np < (ReportInterval / SamplesPerPacket) * ManyReports; np++) {
        if (np % (ReportInterval / SamplesPerPacket) == 0) {
            control_writer.write_receiver_report(
                packet::unix_2_ntp(frame_writer.refresh_ts()), packet_sample_spec);
        }

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
            refresh_sink(sender, frame_writer.refresh_ts());
        }

        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);

        if (np > next_report) {
            control_reader.read_report();

            CHECK(!control_reader.has_src_addr());
            CHECK(control_reader.has_dst_addr(dst_addr2));
            CHECK(control_reader.has_sr(send_src_id));
            CHECK(!control_reader.has_rr());
            CHECK(!control_reader.has_rrtr());
            if (n_reports == 0) {
                CHECK(!control_reader.has_dlrr());
            } else {
                CHECK(control_reader.has_dlrr(send_src_id, recv_src_id));
            }
            CHECK(!control_reader.has_measurement_info());
            CHECK(!control_reader.has_delay_metrics());
            CHECK(!control_reader.has_queue_metrics());

            next_report = np + ReportInterval / SamplesPerPacket;
            n_reports++;
        }
    }
}

// Check reports generated by sender when there are two discovered receivers.
// Generated reports should have blocks dedicated for both receivers.
TEST(sender_sink, reports_two_receivers) {
    enum { MaxParties = 10 };

    init_with_defaults();

    packet::FifoQueue queue;

    SenderSink sender(make_config(), processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderSlot* slot = create_slot(sender);

    packet::stream_source_t send_src_id = 0;
    packet::stream_source_t recv_src_id1 = 0;
    packet::stream_source_t recv_src_id2 = 0;

    {
        SenderSlotMetrics slot_metrics;
        slot->get_metrics(slot_metrics, NULL, NULL);
        send_src_id = slot_metrics.source_id;
        recv_src_id1 = slot_metrics.source_id + 7777;
        recv_src_id2 = slot_metrics.source_id + 9999;
    }

    create_transport_endpoint(slot, address::Iface_AudioSource, proto, dst_addr1, queue);

    packet::FifoQueue control_outbound_queue;
    packet::IWriter* control_endpoint =
        create_control_endpoint(slot, address::Iface_AudioControl, address::Proto_RTCP,
                                dst_addr2, control_outbound_queue);

    test::FrameWriter frame_writer(sender, frame_factory);

    test::PacketReader packet_reader(arena, queue, encoding_map, packet_factory,
                                     dst_addr1, PayloadType_Ch2);

    test::ControlWriter control_writer1(*control_endpoint, packet_factory, dst_addr2,
                                        src_addr1);

    test::ControlWriter control_writer2(*control_endpoint, packet_factory, dst_addr2,
                                        src_addr2);

    control_writer1.set_local_source(recv_src_id1);
    control_writer1.set_remote_source(send_src_id);

    control_writer2.set_local_source(recv_src_id2);
    control_writer2.set_remote_source(send_src_id);

    test::ControlReader control_reader(control_outbound_queue);

    const core::nanoseconds_t unix_base = 1000000000000000;

    size_t next_report = ReportInterval / SamplesPerPacket;
    size_t n_reports = 0;

    for (size_t np = 0; np < (ReportInterval / SamplesPerPacket) * ManyReports; np++) {
        if (np % (ReportInterval / SamplesPerPacket) == 0) {
            control_writer1.write_receiver_report(
                packet::unix_2_ntp(frame_writer.refresh_ts()), packet_sample_spec);
            control_writer2.write_receiver_report(
                packet::unix_2_ntp(frame_writer.refresh_ts()), packet_sample_spec);
        }

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_writer.write_samples(SamplesPerFrame, input_sample_spec, unix_base);
            refresh_sink(sender, frame_writer.refresh_ts());
        }

        packet_reader.read_packet(SamplesPerPacket, packet_sample_spec, unix_base);

        if (np > next_report) {
            control_reader.read_report();

            CHECK(!control_reader.has_src_addr());
            CHECK(control_reader.has_dst_addr(dst_addr2));
            CHECK(control_reader.has_sr(send_src_id));
            CHECK(!control_reader.has_rr());
            CHECK(!control_reader.has_rrtr());
            if (n_reports == 0) {
                CHECK(!control_reader.has_dlrr());
            } else {
                CHECK(control_reader.has_dlrr(send_src_id, recv_src_id1));
                CHECK(control_reader.has_dlrr(send_src_id, recv_src_id2));
            }
            CHECK(!control_reader.has_measurement_info());
            CHECK(!control_reader.has_delay_metrics());
            CHECK(!control_reader.has_queue_metrics());

            next_report = np + ReportInterval / SamplesPerPacket;
            n_reports++;
        }
    }
}

} // namespace pipeline
} // namespace roc
