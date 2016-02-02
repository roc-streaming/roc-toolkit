/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_config/config.h"
#include "roc_core/scoped_ptr.h"
#include "roc_rtp/parser.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_pipeline/receiver.h"

#include "test_packet_stream.h"
#include "test_sample_stream.h"
#include "test_sample_queue.h"

namespace roc {
namespace test {

using namespace pipeline;

using datagram::IDatagramPtr;

TEST_GROUP(receiver) {
    enum {
        // No FEC and resampling.
        ReceiverOptions = 0,

        // Number of samples in every channel per tick.
        TickSamples = SampleStream::ReadBufsz,

        // Number of samples in every channel per packet.
        PktSamples = TickSamples * 5,

        // Latency.
        LatencySamples = PktSamples * 10,

        // Number of ticks without packets after wich session is terminated.
        TimeoutTicks = LatencySamples / PktSamples * 7,

        // Number of packets enought to start rendering.
        EnoughPackets = LatencySamples / PktSamples + 1,

        // Maximum number of packets.
        MaxPackets = ROC_CONFIG_MAX_SESSION_PACKETS
    };

    SampleQueue<(MaxPackets + 1) * PktSamples / TickSamples> output;

    datagram::DatagramQueue input;

    rtp::Parser parser;

    core::ScopedPtr<Receiver> receiver;

    void setup() {
        ReceiverConfig config;

        config.options = ReceiverOptions;
        config.channels = ChannelMask;
        config.session_timeout = TimeoutTicks * TickSamples;
        config.session_latency = LatencySamples;
        config.output_latency = 0;
        config.samples_per_tick = TickSamples;

        receiver.reset(new Receiver(input, output, config));
    }

    void teardown() {
        LONGS_EQUAL(0, output.size());
    }

    void add_port(datagram::port_t port) {
        receiver->add_port(new_address(port), parser);
    }

    void render(size_t n_samples) {
        CHECK(n_samples % TickSamples == 0);

        for (size_t n = 0; n < n_samples / TickSamples; n++) {
            CHECK(receiver->tick());
        }
    }

    void expect_num_sessions(size_t n_sessions) {
        LONGS_EQUAL(n_sessions, receiver->num_sessions());
    }
};

TEST(receiver, no_sessions) {
    SampleStream ss;

    for (size_t n = 0; n < EnoughPackets; n++) {
        render(TickSamples);
        expect_num_sessions(0);

        ss.read_zeros(output, TickSamples);
    }
}

TEST(receiver, no_parsers) {
    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    SampleStream ss;

    for (size_t n = 0; n < EnoughPackets; n++) {
        render(TickSamples);
        expect_num_sessions(0);

        ss.read_zeros(output, TickSamples);
    }
}

TEST(receiver, one_session) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(1);

    SampleStream ss;
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, one_session_long_run) {
    enum { NumIterations = 10 };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    SampleStream ss;

    for (size_t i = 0; i < NumIterations; i++) {
        for (size_t p = 0; p < EnoughPackets; p++) {
            render(PktSamples);
            expect_num_sessions(1);

            ss.read(output, PktSamples);
            ps.write(input, 1, PktSamples);
        }
    }
}

TEST(receiver, wait_min_input_size) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    SampleStream ss;

    for (size_t p = 0; p < EnoughPackets; p++) {
        render(PktSamples);
        ss.read_zeros(output, PktSamples);

        ps.write(input, 1, PktSamples);
    }

    render(EnoughPackets * PktSamples);
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, wait_min_input_size_timeout) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, 1, PktSamples);

    SampleStream ss;

    for (size_t n = 0; n < TimeoutTicks - 1; n++) {
        render(TickSamples);
        expect_num_sessions(1);

        ss.read_zeros(output, TickSamples);
    }

    render(TickSamples);
    expect_num_sessions(0);

    ss.read_zeros(output, TickSamples);
}

TEST(receiver, wait_next_packet_timeout) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    SampleStream ss;

    ps.write(input, EnoughPackets, PktSamples);

    for (size_t p = 0; p < EnoughPackets; p++) {
        render(PktSamples);
        expect_num_sessions(1);

        ss.read(output, PktSamples);
    }

    size_t n_ticks = 0;

    for (; receiver->num_sessions() != 0; n_ticks++) {
        render(TickSamples);
        ss.read_zeros(output, TickSamples);
    }

    CHECK(n_ticks < TimeoutTicks);
}

TEST(receiver, two_sessions_synchronous) {
    add_port(PacketStream::DstPort);

    PacketStream ps1;
    PacketStream ps2;

    ps1.src += 1;
    ps2.src += 2;

    ps1.write(input, EnoughPackets, PktSamples);
    ps2.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(2);

    SampleStream ss;
    ss.set_sessions(2);
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, two_sessions_overlapping) {
    add_port(PacketStream::DstPort);

    PacketStream ps1;
    ps1.src++;
    ps1.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(1);

    SampleStream ss;
    ss.read(output, EnoughPackets * PktSamples);

    PacketStream ps2 = ps1;
    ps2.src++;
    ps2.sn += 10;
    ps2.ts += 10 * PktSamples;

    ps1.write(input, EnoughPackets, PktSamples);
    ps2.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(2);

    ss.set_sessions(2);
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, two_sessions_2sendaddr_1recvaddr_1source) {
    // Two sessions has:
    //  - different source addresses
    //  - same destination addresses
    //  - same source ids

    PacketStream ps1;
    ps1.sid = 11;
    ps1.src = 22;
    ps1.dst = 33;

    PacketStream ps2;
    ps1.sid = 11;
    ps2.src = 44;
    ps2.dst = 33;

    add_port(ps1.dst);

    ps1.write(input, EnoughPackets, PktSamples);
    ps2.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(2);

    SampleStream ss;
    ss.set_sessions(2);
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, two_sessions_1sendaddr_2recvaddr_1source) {
    // Two sessions has:
    //  - same source addresses
    //  - different destination addresses
    //  - same source ids

    PacketStream ps1;
    ps1.sid = 11;
    ps1.src = 22;
    ps1.dst = 33;

    PacketStream ps2;
    ps1.sid = 11;
    ps2.src = 22;
    ps2.dst = 44;

    add_port(ps1.dst);
    add_port(ps2.dst);

    ps1.write(input, EnoughPackets, PktSamples);
    ps2.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(2);

    SampleStream ss;
    ss.set_sessions(2);
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, two_sessions_1sendaddr_1recvaddr_2source) {
    // Two sessions has:
    //  - same source addresses
    //  - same destination addresses
    //  - different source ids

    PacketStream ps1;
    ps1.sid = 11;
    ps1.src = 33;
    ps1.dst = 33;

    PacketStream ps2;
    ps1.sid = 22;
    ps2.src = 33;
    ps2.dst = 33;

    add_port(ps1.dst);

    ps1.write(input, EnoughPackets, PktSamples);
    ps2.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(2);

    SampleStream ss;
    ss.set_sessions(2);
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, drop_above_max_sessions) {
    enum { MaxSessions = ROC_CONFIG_MAX_SESSIONS };

    add_port(PacketStream::DstPort);

    for (datagram::port_t n = 0; n < MaxSessions; n++) {
        PacketStream ps;
        ps.src += n;
        ps.write(input, 1, PktSamples);

        render(PktSamples);
        expect_num_sessions(n + 1);
    }

    expect_num_sessions(MaxSessions);

    PacketStream ps;
    ps.src += MaxSessions;
    ps.write(input, 1, PktSamples);

    render(PktSamples);
    expect_num_sessions(MaxSessions);

    output.clear();
}

TEST(receiver, drop_above_max_packets) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, MaxPackets + 1, PktSamples);

    render(MaxPackets * PktSamples);

    SampleStream ss;
    ss.read(output, MaxPackets * PktSamples);

    ps.write(input, 1, PktSamples);
    render(PktSamples * 2);

    ss.read_zeros(output, PktSamples);
    ss.advance(PktSamples);

    ss.read(output, PktSamples);
}

TEST(receiver, seqnum_overflow) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.sn = packet::seqnum_t(-1) - 3;

    ps.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);

    SampleStream ss;
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, seqnum_reorder) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.sn = 10000;
    ps.ts = 100000;
    ps.value += PktSamples * (EnoughPackets - 1);

    for (size_t p = EnoughPackets; p > 0; p--) {
        input.write(ps.make(PktSamples));
        ps.sn--;
        ps.ts -= PktSamples;
        ps.value -= PktSamples;
    }

    render(EnoughPackets * PktSamples);

    SampleStream ss;
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, seqnum_drop_late) {
    enum { NumDelayed = 5 };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets - NumDelayed, PktSamples);

    // store position of delayed packets
    PacketStream delayed = ps;

    // skip delayed packets now
    ps.sn += NumDelayed;
    ps.ts += NumDelayed * PktSamples;
    ps.value += NumDelayed * PktSamples;

    // write more packets
    ps.write(input, EnoughPackets, PktSamples);
    render(EnoughPackets * PktSamples);

    SampleStream ss;

    // read samples before delayed packets
    ss.read(output, (EnoughPackets - NumDelayed) * PktSamples);

    // read zeros instead of delayed packets
    ss.read_zeros(output, NumDelayed * PktSamples);
    ss.advance(NumDelayed * PktSamples);

    // write delayed packets
    delayed.write(input, NumDelayed, PktSamples);
    render(EnoughPackets * PktSamples * 2);

    // read samples after delayed packets (delayed packets are ignored)
    ss.read(output, EnoughPackets * PktSamples);

    // ensure there are no more samples
    ss.read_zeros(output, EnoughPackets * PktSamples);
}

TEST(receiver, seqnum_ignore_gap) {
    enum { Gap = 33 };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    ps.sn += Gap;
    ps.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * 2 * PktSamples);

    SampleStream ss;
    ss.read(output, EnoughPackets * 2 * PktSamples);
}

TEST(receiver, seqnum_shutdown_on_jump) {
    enum { Jump = ROC_CONFIG_MAX_SN_JUMP + 1 };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    ps.sn += Jump;
    ps.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples + TickSamples);
    expect_num_sessions(1);

    SampleStream ss;

    ss.read(output, EnoughPackets * PktSamples);
    ss.read_zeros(output, TickSamples);

    render(TickSamples);
    expect_num_sessions(0);

    ss.read_zeros(output, TickSamples);
}

TEST(receiver, timestamp_overflow) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.ts = packet::timestamp_t(-1) - 33;

    ps.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);

    SampleStream ss;
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, timestamp_zeros_on_late) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    packet::timestamp_t late = ps.ts;

    ps.ts += PktSamples;
    ps.value += PktSamples;
    ps.write(input, EnoughPackets, PktSamples);

    ps.ts = late;
    ps.write(input, 1, PktSamples);

    render((EnoughPackets * 3 + 1) * PktSamples);

    SampleStream ss;

    ss.read(output, EnoughPackets * PktSamples);

    ss.read_zeros(output, PktSamples);
    ss.advance(PktSamples);

    ss.read(output, EnoughPackets * PktSamples);

    ss.read_zeros(output, EnoughPackets * PktSamples);
}

TEST(receiver, timestamp_zeros_on_gap) {
    enum { Gap = 10 };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    ps.ts += Gap * PktSamples;
    ps.value += Gap * PktSamples;

    ps.write(input, EnoughPackets, PktSamples);

    render((EnoughPackets * 2 + Gap) * PktSamples);

    SampleStream ss;

    ss.read(output, EnoughPackets * PktSamples);

    ss.read_zeros(output, Gap * PktSamples);
    ss.advance(Gap * PktSamples);

    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, timestamp_overlapping) {
    enum { Overlap = PktSamples / 2 };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    ps.ts -= Overlap;
    ps.value -= Overlap;

    ps.write(input, EnoughPackets, PktSamples);
    ps.write(input, 1, PktSamples - Overlap);

    render((EnoughPackets * 2 + 1) * PktSamples);

    SampleStream ss;

    ss.read(output, EnoughPackets * 2 * PktSamples);
    ss.read_zeros(output, PktSamples);
}

TEST(receiver, timestamp_shutdown_on_jump) {
    enum { Jump = ROC_CONFIG_MAX_TS_JUMP + 1 };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    ps.ts += Jump;
    ps.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * PktSamples);
    expect_num_sessions(1);

    SampleStream ss;
    ss.read(output, EnoughPackets * PktSamples);

    render(PktSamples);
    expect_num_sessions(0);

    ss.read_zeros(output, PktSamples);
}

TEST(receiver, tiny_packets) {
    CHECK(TickSamples % 2 == 0);

    enum {
        TinyPacketSamples = TickSamples / 2,
        TinyPackets = EnoughPackets * (PktSamples / TinyPacketSamples)
    };

    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, TinyPackets, TinyPacketSamples);

    render(TinyPackets * TinyPacketSamples);

    SampleStream ss;
    ss.read(output, TinyPackets * TinyPacketSamples);
}

TEST(receiver, non_aligned_packets) {
    CHECK(PktSamples % 2 == 0);

    add_port(PacketStream::DstPort);

    PacketStream ps;

    ps.write(input, 1, PktSamples / 2);
    ps.write(input, 1, PktSamples);
    ps.write(input, 1, PktSamples / 2);

    ps.write(input, EnoughPackets - 2, PktSamples);

    render(EnoughPackets * PktSamples);

    SampleStream ss;
    ss.read(output, EnoughPackets * PktSamples);
}

TEST(receiver, corrupted_packet_drop_new_session) {
    add_port(PacketStream::DstPort);

    PacketStream ps;

    IDatagramPtr corrupted = ps.make(*new_byte_buffer<1>());

    input.write(corrupted);

    render(TickSamples);
    expect_num_sessions(0);

    IDatagramPtr good = ps.make(1);
    input.write(good);

    render(TickSamples);
    expect_num_sessions(1);

    output.clear();
}

TEST(receiver, corrupted_packet_ignore_in_existing_session) {
    add_port(PacketStream::DstPort);

    PacketStream ps;
    ps.write(input, EnoughPackets, PktSamples);

    IDatagramPtr corrupted = ps.make(*new_byte_buffer<1>());
    input.write(corrupted);

    ps.write(input, EnoughPackets, PktSamples);

    render(EnoughPackets * 2 * PktSamples);

    SampleStream ss;
    ss.read(output, EnoughPackets * 2 * PktSamples);
}

} // namespace test
} // namespace roc
