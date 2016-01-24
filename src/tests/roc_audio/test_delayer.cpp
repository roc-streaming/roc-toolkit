/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/delayer.h"

#include "test_helpers.h"

namespace roc {
namespace test {

using namespace packet;
using namespace audio;

namespace {

enum { ChMask = 0x3, NumChannels = 2, NumSamples = 100, NumPackets = 5 };

enum { Rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE };

} // namespace

TEST_GROUP(delayer) {
    IPacketPtr make(seqnum_t sn) {
        IPacketPtr packet = new_audio_packet();

        packet->rtp()->set_seqnum(sn);
        packet->rtp()->set_timestamp(packet::timestamp_t(sn* NumSamples));

        sample_t samples[NumSamples * NumChannels] = {};
        packet->audio()->configure(ChMask, NumSamples, Rate);
        packet->audio()->write_samples(ChMask, 0, samples, NumSamples);

        return packet;
    }
};

TEST(delayer, no_delay) {
    PacketQueue queue;
    Delayer delayer(queue, 0);

    CHECK(!delayer.read());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        IPacketPtr packet = make(n);
        queue.write(packet);
        CHECK(delayer.read() == packet);
    }
}

TEST(delayer, delay1) {
    PacketQueue queue;
    Delayer delayer(queue, NumSamples * (NumPackets - 1));

    IPacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(!delayer.read());
        packets[n] = make(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(delayer.read() == packets[n]);
    }

    CHECK(!delayer.read());

    for (seqnum_t n = 0; n < NumPackets; n++) {
        IPacketPtr packet = make(NumPackets + n);
        queue.write(packet);
        CHECK(delayer.read() == packet);
    }

    CHECK(!delayer.read());
}

TEST(delayer, delay2) {
    PacketQueue queue;
    Delayer delayer(queue, NumSamples * (NumPackets - 1));

    IPacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = make(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(delayer.read() == packets[n]);
    }

    CHECK(!delayer.read());
}

TEST(delayer, late_duplicates) {
    PacketQueue queue;
    Delayer delayer(queue, NumSamples * (NumPackets - 1));

    IPacketPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = make(n);
        queue.write(packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        CHECK(delayer.read() == packets[n]);
    }

    for (seqnum_t n = 0; n < NumPackets; n++) {
        IPacketPtr packet = make(n);
        queue.write(packet);
        CHECK(delayer.read() == packet);
    }

    CHECK(!delayer.read());
}

} // namespace test
} // namespace roc
