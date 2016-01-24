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

#include "roc_packet/packet_queue.h"
#include "roc_packet/watchdog.h"

#include "test_packet.h"

namespace roc {
namespace test {

using namespace packet;

TEST_GROUP(watchdog) {
    enum {
        NumSamples = 103,
        Timeout = 33,
        Rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE,
        SnJump = ROC_CONFIG_MAX_SN_JUMP,
        TsJump = ROC_CONFIG_MAX_TS_JUMP
    };

    PacketQueue queue;

    core::ScopedPtr<Watchdog> watchdog;

    void setup() {
        watchdog.reset(new Watchdog(queue, Timeout, Rate));
    }

    IPacketConstPtr add_packet(source_t src, seqnum_t sn, timestamp_t ts) {
        IPacketPtr pkt = new_audio_packet(src, sn, ts);
        pkt->audio()->configure(0x1, 1, Rate);
        queue.write(pkt);
        return pkt;
    }
};

TEST(watchdog, no_packets) {
    CHECK(watchdog->update());
    CHECK(!watchdog->read());
}

TEST(watchdog, read) {
    enum { NumPackets = Timeout * 10 };

    IPacketConstPtr packets[NumPackets];

    for (seqnum_t n = 0; n < NumPackets; n++) {
        packets[n] = add_packet(0, n, n * NumSamples);
    }

    for (size_t n = 0; n < NumPackets; n++) {
        CHECK(watchdog->update());
        CHECK(watchdog->read() == packets[n]);
    }

    CHECK(watchdog->update());
    CHECK(!watchdog->read());
}

TEST(watchdog, timeout) {
    enum { NumPackets = 7 };

    for (seqnum_t n = 0; n < NumPackets; n++) {
        IPacketConstPtr packet = add_packet(0, n, n * NumSamples);
        CHECK(watchdog->update());
        CHECK(watchdog->read() == packet);
    }

    for (size_t n = 0; n < Timeout; n++) {
        CHECK(watchdog->update());
        CHECK(!watchdog->read());
    }

    add_packet(0, NumPackets, NumPackets * NumSamples);

    CHECK(!watchdog->update());
    CHECK(!watchdog->read());
}

TEST(watchdog, seqnum_overflow) {
    const seqnum_t sn1 = seqnum_t(-1) - SnJump / 2;
    const seqnum_t sn2 = seqnum_t(sn1 + SnJump);

    IPacketConstPtr p1 = add_packet(0, sn1, NumSamples * 2);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, sn2, NumSamples * 3);
    CHECK(watchdog->read() == p2);
    CHECK(watchdog->update());
}

TEST(watchdog, seqnum_jump_gt) {
    const seqnum_t sn1 = seqnum_t(-1) - SnJump / 2;
    const seqnum_t sn2 = seqnum_t(sn1 + SnJump + 1);

    IPacketConstPtr p1 = add_packet(0, sn1, NumSamples * 2);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, sn2, NumSamples * 3);
    CHECK(!watchdog->read());
    CHECK(!watchdog->update());
}

TEST(watchdog, seqnum_jump_lt) {
    const seqnum_t sn1 = seqnum_t(-1) - SnJump / 2;
    const seqnum_t sn2 = seqnum_t(sn1 + SnJump + 1);

    IPacketConstPtr p1 = add_packet(0, sn2, NumSamples * 3);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, sn1, NumSamples * 2);
    CHECK(!watchdog->read());
    CHECK(!watchdog->update());
}

TEST(watchdog, seqnum_late) {
    const seqnum_t sn1 = 100;
    const seqnum_t sn2 = 50;
    const seqnum_t sn3 = sn2 + SnJump + 1;

    IPacketConstPtr p1 = add_packet(0, sn1, NumSamples * 2);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, sn2, NumSamples * 3);
    CHECK(watchdog->read() == p2);
    CHECK(watchdog->update());

    IPacketConstPtr p3 = add_packet(0, sn3, NumSamples * 4);
    CHECK(watchdog->read() == p3);
    CHECK(watchdog->update());
}

TEST(watchdog, timestamp_overflow) {
    const timestamp_t ts1 = timestamp_t(-1) - TsJump / 2;
    const timestamp_t ts2 = ts1 + TsJump;

    IPacketConstPtr p1 = add_packet(0, 2, ts1);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, 3, ts2);
    CHECK(watchdog->read() == p2);
    CHECK(watchdog->update());
}

TEST(watchdog, timestamp_jump_gt) {
    const timestamp_t ts1 = timestamp_t(-1) - TsJump / 2;
    const timestamp_t ts2 = ts1 + TsJump + 1;

    IPacketConstPtr p1 = add_packet(0, 2, ts1);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, 3, ts2);
    CHECK(!watchdog->read());
    CHECK(!watchdog->update());
}

TEST(watchdog, timestamp_jump_lt) {
    const timestamp_t ts1 = timestamp_t(-1) - TsJump / 2;
    const timestamp_t ts2 = ts1 + TsJump + 1;

    IPacketConstPtr p1 = add_packet(0, 2, ts2);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, 3, ts1);
    CHECK(!watchdog->read());
    CHECK(!watchdog->update());
}

TEST(watchdog, timestamp_late) {
    const timestamp_t ts1 = 100;
    const timestamp_t ts2 = 50;
    const timestamp_t ts3 = ts2 + TsJump + 1;

    IPacketConstPtr p1 = add_packet(0, 1, ts1);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(0, 2, ts2);
    CHECK(watchdog->read() == p2);
    CHECK(watchdog->update());

    IPacketConstPtr p3 = add_packet(0, 3, ts3);
    CHECK(!watchdog->read());
    CHECK(!watchdog->update());
}

TEST(watchdog, source_id_jump) {
    IPacketConstPtr p1 = add_packet(111, 1, 1);
    CHECK(watchdog->read() == p1);
    CHECK(watchdog->update());

    IPacketConstPtr p2 = add_packet(222, 2, 2);
    CHECK(!watchdog->read());
    CHECK(!watchdog->update());
}

} // namespace test
} // namespace roc
