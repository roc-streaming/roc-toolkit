/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/format_map.h"

#include "test_frame_reader.h"
#include "test_frame_writer.h"
#include "test_packet_sender.h"

namespace roc {
namespace pipeline {

namespace {

enum {
    MaxBufSize = 500,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

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
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, true);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);
fec::CodecMap codec_map;
rtp::FormatMap format_map;

} // namespace

TEST_GROUP(sender_sink_receiver_source) {
    bool is_fec_supported(int flags) {
        if (flags & FlagReedSolomon) {
            return codec_map.is_supported(packet::FEC_ReedSolomon_M8);
        }
        if (flags & FlagLDPC) {
            return codec_map.is_supported(packet::FEC_LDPC_Staircase);
        }
        return true;
    }

    void send_receive(int flags, size_t num_sessions) {
        packet::Queue queue;

        PortConfig source_port = sender_source_port(flags);
        PortConfig repair_port = sender_repair_port(flags);

        SenderSink sender(sender_config(flags),
                          source_port,
                          queue,
                          repair_port,
                          queue,
                          codec_map,
                          format_map,
                          packet_pool,
                          byte_buffer_pool,
                          sample_buffer_pool,
                          allocator);

        CHECK(sender.valid());

        ReceiverSource receiver(receiver_config(),
                                codec_map,
                                format_map,
                                packet_pool,
                                byte_buffer_pool,
                                sample_buffer_pool,
                                allocator);

        CHECK(receiver.valid());

        add_receiver_ports(receiver);

        FrameWriter frame_writer(sender, sample_buffer_pool);

        for (size_t nf = 0; nf < ManyFrames; nf++) {
            frame_writer.write_samples(SamplesPerFrame * NumCh);
        }

        PacketSender packet_sender(packet_pool, receiver);

        filter_packets(flags, queue, packet_sender);

        FrameReader frame_reader(receiver, sample_buffer_pool);

        packet_sender.deliver(Latency / SamplesPerPacket);

        for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
            for (size_t nf = 0; nf < FramesPerPacket; nf++) {
                frame_reader.read_samples(SamplesPerFrame * NumCh, num_sessions);

                UNSIGNED_LONGS_EQUAL(num_sessions, receiver.num_sessions());
            }

            packet_sender.deliver(1);
        }
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

    PortConfig sender_source_port(int flags) {
        PortConfig port_config;
        if (flags & FlagReedSolomon) {
            port_config.address = new_address(20);
            port_config.protocol = Proto_RTP_RSm8_Source;
        } else if (flags & FlagLDPC) {
            port_config.address = new_address(30);
            port_config.protocol = Proto_RTP_LDPC_Source;
        } else {
            port_config.address = new_address(10);
            port_config.protocol = Proto_RTP;
        }
        return port_config;
    }

    PortConfig sender_repair_port(int flags) {
        PortConfig port_config;
        if (flags & FlagReedSolomon) {
            port_config.address = new_address(21);
            port_config.protocol = Proto_RSm8_Repair;
        } else if (flags & FlagLDPC) {
            port_config.address = new_address(31);
            port_config.protocol = Proto_LDPC_Repair;
        } else {
            port_config.protocol = Proto_None;
        }
        return port_config;
    }

    void add_receiver_ports(ReceiverSource& receiver) {
        PortConfig port_config;

        port_config.address = new_address(10);
        port_config.protocol = Proto_RTP;
        CHECK(receiver.add_port(port_config));

        port_config.address = new_address(20);
        port_config.protocol = Proto_RTP_RSm8_Source;
        CHECK(receiver.add_port(port_config));

        port_config.address = new_address(21);
        port_config.protocol = Proto_RSm8_Repair;
        CHECK(receiver.add_port(port_config));

        port_config.address = new_address(30);
        port_config.protocol = Proto_RTP_LDPC_Source;
        CHECK(receiver.add_port(port_config));

        port_config.address = new_address(31);
        port_config.protocol = Proto_LDPC_Repair;
        CHECK(receiver.add_port(port_config));
    }

    SenderConfig sender_config(int flags) {
        SenderConfig config;

        config.input_channels = ChMask;
        config.packet_length = SamplesPerPacket * core::Second / SampleRate;
        config.internal_frame_size = MaxBufSize;

        if (flags & FlagReedSolomon) {
            config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;
        }

        if (flags & FlagLDPC) {
            config.fec_encoder.scheme = packet::FEC_LDPC_Staircase;
        }

        config.fec_writer.n_source_packets = SourcePackets;
        config.fec_writer.n_repair_packets = RepairPackets;

        config.interleaving = (flags & FlagInterleaving);
        config.timing = false;
        config.poisoning = true;

        return config;
    }

    ReceiverConfig receiver_config() {
        ReceiverConfig config;

        config.common.output_sample_rate = SampleRate;
        config.common.output_channels = ChMask;
        config.common.internal_frame_size = MaxBufSize;

        config.common.resampling = false;
        config.common.timing = false;
        config.common.poisoning = true;

        config.default_session.channels = ChMask;

        config.default_session.target_latency = Latency * core::Second / SampleRate;
        config.default_session.watchdog.no_playback_timeout =
            Timeout * core::Second / SampleRate;

        return config;
    }
};

TEST(sender_sink_receiver_source, bare) {
    send_receive(FlagNone, 1);
}

TEST(sender_sink_receiver_source, interleaving) {
    send_receive(FlagInterleaving, 1);
}

TEST(sender_sink_receiver_source, fec_rs) {
    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon, 1);
    }
}

TEST(sender_sink_receiver_source, fec_ldpc) {
    if (is_fec_supported(FlagLDPC)) {
        send_receive(FlagLDPC, 1);
    }
}

TEST(sender_sink_receiver_source, fec_interleaving) {
    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagInterleaving, 1);
    }
}

TEST(sender_sink_receiver_source, fec_loss) {
    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagLosses, 1);
    }
}

TEST(sender_sink_receiver_source, fec_drop_source) {
    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagDropSource, 0);
    }
}

TEST(sender_sink_receiver_source, fec_drop_repair) {
    if (is_fec_supported(FlagReedSolomon)) {
        send_receive(FlagReedSolomon | FlagDropRepair, 1);
    }
}

} // namespace pipeline
} // namespace roc
