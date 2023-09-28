/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_packet/delayed_reader.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/config.h"

namespace roc {
namespace packet {

namespace {

enum { SampleRate = 1000, NumSamples = 100, NumPackets = 30 };

const core::nanoseconds_t NsPerSample = core::Second / SampleRate;
const audio::SampleSpec
    SampleSpecs(SampleRate, audio::ChanLayout_Surround, audio::ChanMask_Surround_Stereo);

core::HeapArena arena;
PacketFactory packet_factory(arena);

PacketPtr new_packet(seqnum_t sn) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP);
    packet->rtp()->seqnum = sn;
    packet->rtp()->stream_timestamp = stream_timestamp_t(sn * NumSamples);

    return packet;
}

} // namespace

TEST_GROUP(delayed_reader) {};

TEST(delayed_reader, no_delay) {
    Queue queue;
    DelayedReader dr(queue, 0, SampleSpecs);

    CHECK(!dr.read());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr packet = new_packet(n);
        queue.write(packet);
        CHECK(dr.read() == packet);
    }
}

TEST(delayed_reader, delay) {
    Queue queue;
    DelayedReader dr(queue, NumSamples * (NumPackets - 1) * NsPerSample, SampleSpecs);

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(!dr.read());
        packets[n] = new_packet(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(dr.read() == packets[n]);
    }

    CHECK(!dr.read());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr packet = new_packet(NumPackets + n);
        queue.write(packet);
        CHECK(dr.read() == packet);
    }

    CHECK(!dr.read());
}

TEST(delayed_reader, instant) {
    Queue queue;
    DelayedReader dr(queue, NumSamples * (NumPackets - 1) * NsPerSample, SampleSpecs);

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(dr.read() == packets[n]);
    }

    CHECK(!dr.read());
}

TEST(delayed_reader, trim) {
    Queue queue;
    DelayedReader dr(queue, NumSamples * (NumPackets - 1) * NsPerSample, SampleSpecs);

    PacketPtr packets[NumPackets * 2];

    for (seqnum_t n = 0; n < NumPackets * 2; n++) {
        packets[n] = new_packet(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = NumPackets; n < NumPackets * 2; n++) {
        CHECK(dr.read() == packets[n]);
    }

    CHECK(!dr.read());
}

TEST(delayed_reader, late_duplicates) {
    Queue queue;
    DelayedReader dr(queue, NumSamples * (NumPackets - 1) * NsPerSample, SampleSpecs);

    PacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = new_packet(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(dr.read() == packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        PacketPtr packet = new_packet(n);
        queue.write(packet);
        CHECK(dr.read() == packet);
    }

    CHECK(!dr.read());
}

} // namespace packet
} // namespace roc
