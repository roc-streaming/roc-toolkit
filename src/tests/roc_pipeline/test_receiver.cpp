/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/parse_address.h"
#include "roc_pipeline/receiver.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/pcm_encoder.h"

#include "test_frame_reader.h"
#include "test_packet_writer.h"

namespace roc {
namespace pipeline {

namespace {

rtp::PayloadType PayloadType = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 4096,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    Latency = SamplesPerPacket * 7,
    Timeout = Latency * 13,

    ManyPackets = Latency / SamplesPerPacket * 10,

    MaxSnJump = ManyPackets * 5,
    MaxTsJump = ManyPackets * 7 * SamplesPerPacket
};

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, 1);
packet::PacketPool packet_pool(allocator, 1);

rtp::FormatMap format_map;
rtp::Composer rtp_composer(NULL);
rtp::PCMEncoder<int16_t, NumCh> pcm_encoder;

} // namespace

TEST_GROUP(receiver) {
    ReceiverConfig config;

    packet::Address src1;
    packet::Address src2;

    PortConfig port1;
    PortConfig port2;

    void setup() {
        config.sample_rate = SampleRate;
        config.channels = ChMask;

        config.default_session.channels = ChMask;
        config.default_session.samples_per_packet = SamplesPerPacket;
        config.default_session.latency = Latency;
        config.default_session.timeout = Timeout;
        config.default_session.payload_type = PayloadType;

        config.default_session.fec.codec = fec::NoCodec;

        config.default_session.validator.max_sn_jump = MaxSnJump;
        config.default_session.validator.max_ts_jump = MaxTsJump * 1000 / SampleRate;

        src1 = new_address(1);
        src2 = new_address(2);

        port1.address = new_address(3);
        port1.protocol = Proto_RTP;

        port2.address = new_address(4);
        port2.protocol = Proto_RTP;
    }
};

TEST(receiver, no_sessions) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);

        UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
    }
}

TEST(receiver, no_ports) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);

        UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
    }
}

TEST(receiver, one_session) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver, one_session_long_run) {
    enum { NumIterations = 10 };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (size_t np = 0; np < ManyPackets; np++) {
            for (size_t nf = 0; nf < FramesPerPacket; nf++) {
                frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

                UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
            }

            packet_writer.write_packets(1, SamplesPerPacket, ChMask);
        }
    }
}

TEST(receiver, initial_latency) {
    enum { NumPackets = Latency / SamplesPerPacket };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t np = 0; np < NumPackets - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame * NumCh);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < NumPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver, initial_latency_timeout) {
    enum { NumPackets = Timeout / SamplesPerPacket };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    packet_writer.write_packets(1, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < NumPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame * NumCh);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    frame_reader.skip_zeros(SamplesPerFrame * NumCh);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

TEST(receiver, timeout) {
    enum { NumPackets = Latency / SamplesPerPacket };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(NumPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t np = 0; np < NumPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);
    }
}

TEST(receiver, two_sessions_synchronous) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer1(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src1, port1.address);

    PacketWriter packet_writer2(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src2, port1.address);

    for (size_t np = 0; np < ManyPackets; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 2);

        UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
    }
}

TEST(receiver, two_sessions_overlapping) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer1(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer1.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    PacketWriter packet_writer2(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src2, port1.address);

    packet_writer2.set_offset(packet_writer1.offset());

    for (size_t np = 0; np < ManyPackets; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 2);

        UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
    }
}

TEST(receiver, two_sessions_two_ports) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    CHECK(receiver.add_port(port1));
    CHECK(receiver.add_port(port2));

    PacketWriter packet_writer1(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src1, port1.address);

    PacketWriter packet_writer2(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src2, port2.address);

    for (size_t np = 0; np < ManyPackets; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 2);

        UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
    }
}

TEST(receiver, two_sessions_same_address_same_stream) {
    enum { Offset = 7 };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    CHECK(receiver.add_port(port1));
    CHECK(receiver.add_port(port2));

    PacketWriter packet_writer1(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src1, port1.address);

    PacketWriter packet_writer2(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src1, port2.address);

    packet_writer1.set_source(11);
    packet_writer2.set_source(11);

    packet_writer2.set_offset(77);

    for (size_t np = 0; np < ManyPackets; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver, two_sessions_same_address_different_streams) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    CHECK(receiver.add_port(port1));
    CHECK(receiver.add_port(port2));

    PacketWriter packet_writer1(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src1, port1.address);

    PacketWriter packet_writer2(receiver, rtp_composer, pcm_encoder, packet_pool,
                                byte_buffer_pool, PayloadType, src1, port2.address);

    packet_writer1.set_source(11);
    packet_writer2.set_source(22);

    packet_writer2.set_offset(77);
    packet_writer2.set_seqnum(5);
    packet_writer2.set_timestamp(5 * SamplesPerPacket);

    for (size_t np = 0; np < ManyPackets; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver, seqnum_overflow) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.set_seqnum(packet::seqnum_t(-1) - ManyPackets / 2);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }
}

TEST(receiver, seqnum_small_jump) {
    enum { ShiftedPackets = 5 };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    packet_writer.set_seqnum(ManyPackets + ShiftedPackets);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * 2 * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
}

TEST(receiver, seqnum_large_jump) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    packet_writer.set_seqnum(ManyPackets + MaxSnJump);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);
    }
}

TEST(receiver, seqnum_reorder) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    for (ssize_t np = ManyPackets - 1; np >= 0; np--) {
        packet_writer.shift_to(size_t(np), SamplesPerPacket, ChMask);
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }
}

TEST(receiver, seqnum_late) {
    enum { DelayedPackets = 5 };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets - DelayedPackets, SamplesPerPacket, ChMask);

    packet_writer.shift_to(ManyPackets, SamplesPerPacket, ChMask);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < (ManyPackets - DelayedPackets) * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    for (size_t nf = 0; nf < DelayedPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
    }

    packet_writer.shift_to(ManyPackets - DelayedPackets, SamplesPerPacket, ChMask);
    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, ChMask);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
}

TEST(receiver, timestamp_overflow) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.set_timestamp(packet::timestamp_t(-1)
                                - ManyPackets * SamplesPerPacket / 2);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }
}

TEST(receiver, timestamp_small_jump) {
    enum { ShiftedPackets = 5 };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets - ShiftedPackets, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp(ManyPackets * SamplesPerPacket);
    packet_writer.set_offset(ManyPackets * SamplesPerPacket * NumCh);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < (ManyPackets - ShiftedPackets) * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    for (size_t nf = 0; nf < ShiftedPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
    }

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }
}

TEST(receiver, timestamp_large_jump) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp((ManyPackets + 1) * SamplesPerPacket + Timeout);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);
    }
}

TEST(receiver, timestamp_overlap) {
    enum { Overlap = SamplesPerPacket / 2 };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp(ManyPackets * SamplesPerPacket - Overlap);
    packet_writer.set_offset((ManyPackets * SamplesPerPacket - Overlap) * NumCh);

    packet_writer.write_packets(ManyPackets + 1, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * 2 * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }
}

TEST(receiver, timestamp_reorder) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    for (ssize_t np = ManyPackets - 1; np >= 0; np--) {
        packet_writer.set_offset(
            packet::timestamp_t((ManyPackets + np) * SamplesPerPacket * NumCh));

        packet_writer.set_timestamp(
            packet::timestamp_t((ManyPackets + np) * SamplesPerPacket));

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    for (size_t nf = 0; nf < (ManyPackets - 1) * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
    }

    for (size_t nf = 0; nf < FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);
    }
}

TEST(receiver, timestamp_late) {
    enum { DelayedPackets = 5 };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyPackets - DelayedPackets, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp(ManyPackets * SamplesPerPacket);
    packet_writer.set_offset(ManyPackets * SamplesPerPacket * NumCh);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < (ManyPackets - DelayedPackets) * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    for (size_t nf = 0; nf < DelayedPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
    }

    packet_writer.set_timestamp((ManyPackets - DelayedPackets) * SamplesPerPacket);
    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, ChMask);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
}

TEST(receiver, packet_size_small) {
    enum {
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,
        ManySmallPackets = Latency / SamplesPerSmallPacket * 10
    };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManySmallPackets, SamplesPerSmallPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManySmallPackets / SmallPacketsPerFrame; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
}

TEST(receiver, packet_size_large) {
    enum {
        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,
        ManyLargePackets = Latency / SamplesPerLargePacket * 10
    };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.write_packets(ManyLargePackets, SamplesPerLargePacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyLargePackets * FramesPerLargePacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
}

TEST(receiver, packet_size_variable) {
    enum {
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame
    };

    enum {
        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket
    };

    enum {
        NumPackets = 100,
        NumSamples = NumPackets * (SamplesPerSmallPacket + SamplesPerLargePacket)
    };

    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    for (size_t np = 0; np < NumPackets; np++) {
        packet_writer.write_packets(1, SamplesPerSmallPacket, ChMask);
        packet_writer.write_packets(1, SamplesPerLargePacket, ChMask);
    }

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < NumSamples / SamplesPerFrame; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }
}

TEST(receiver, bad_packet_new_session) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    packet_writer.set_corrupt(true);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);

        UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
    }
}

TEST(receiver, bad_packet_old_session) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    FrameReader frame_reader(receiver, sample_buffer_pool);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    packet_writer.set_corrupt(true);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    packet_writer.set_corrupt(false);
    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
    }

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }
}

TEST(receiver, status) {
    Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);

    CHECK(receiver.valid());
    CHECK(receiver.add_port(port1));

    PacketWriter packet_writer(receiver, rtp_composer, pcm_encoder, packet_pool,
                               byte_buffer_pool, PayloadType, src1, port1.address);

    audio::Frame frame;

    frame.samples =
        new (sample_buffer_pool) core::Buffer<audio::sample_t>(sample_buffer_pool);

    frame.samples.resize(FramesPerPacket * NumCh);

    CHECK(receiver.read(frame) == IReceiver::Inactive);

    packet_writer.write_packets(ManyPackets, SamplesPerPacket, ChMask);

    CHECK(receiver.read(frame) == IReceiver::Active);

    for (;;) {
        if (receiver.read(frame) == IReceiver::Inactive) {
            break;
        }
    }
}

} // namespace pipeline
} // namespace roc
