/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/time.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/config.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/validator.h"

namespace roc {
namespace rtp {

namespace {

const PayloadType Pt1 = PayloadType_L16_Stereo;
const PayloadType Pt2 = PayloadType_L16_Mono;

enum { Src1 = 55, Src2 = 77, SampleRate = 10000, MaxSnJump = 100, MaxTsJump = 1000 };

const audio::SampleSpec
    SampleSpecs(SampleRate, audio::ChanLayout_Surround, audio::ChanMask_Surround_Stereo);

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);

} // namespace

TEST_GROUP(validator) {
    ValidatorConfig config;

    void setup() {
        memset((void*)&config, 0, sizeof(config));
        config.max_sn_jump = MaxSnJump;
        config.max_ts_jump = MaxTsJump * core::Second / SampleRate;
    }

    packet::PacketPtr new_packet(PayloadType pt, packet::stream_source_t src,
                                 packet::seqnum_t sn, packet::stream_timestamp_t ts,
                                 core::nanoseconds_t cts) {
        packet::PacketPtr packet = packet_factory.new_packet();
        CHECK(packet);

        packet->add_flags(packet::Packet::FlagRTP);
        packet->rtp()->payload_type = pt;
        packet->rtp()->source = src;
        packet->rtp()->seqnum = sn;
        packet->rtp()->stream_timestamp = ts;
        packet->rtp()->capture_timestamp = cts;

        return packet;
    }
};

TEST(validator, empty) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    CHECK(!validator.read());
}

TEST(validator, normal) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1, 0);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, 2, 0);
    queue.write(p2);
    CHECK(validator.read() == p2);

    CHECK(!queue.read());
}

TEST(validator, payload_id_jump) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1, 0);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt2, Src1, 2, 2, 0);
    queue.write(p2);
    CHECK(!validator.read());

    CHECK(!queue.read());
}

TEST(validator, source_id_jump) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1, 0);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src2, 2, 2, 0);
    queue.write(p2);
    CHECK(!validator.read());

    CHECK(!queue.read());
}

TEST(validator, seqnum_no_jump) {
    const packet::seqnum_t sns[] = {
        1,
        packet::seqnum_t(-1) - MaxSnJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::seqnum_t sn1 = sns[i];
        const packet::seqnum_t sn2 = packet::seqnum_t(sn1 + MaxSnJump);

        packet::Queue queue;
        Validator validator(queue, config, SampleSpecs);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, sn1, 1, 0);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, sn2, 2, 0);
        queue.write(p2);
        CHECK(validator.read() == p2);

        CHECK(!queue.read());
    }
}

TEST(validator, seqnum_jump_up) {
    const packet::seqnum_t sns[] = {
        1,
        packet::seqnum_t(-1) - MaxSnJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::seqnum_t sn1 = sns[i];
        const packet::seqnum_t sn2 = packet::seqnum_t(sn1 + MaxSnJump + 1);

        packet::Queue queue;
        Validator validator(queue, config, SampleSpecs);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, sn1, 1, 0);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, sn2, 2, 0);
        queue.write(p2);
        CHECK(!validator.read());

        CHECK(!queue.read());
    }
}

TEST(validator, seqnum_jump_down) {
    const packet::seqnum_t sns[] = {
        1,
        packet::seqnum_t(-1) - MaxSnJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::seqnum_t sn1 = sns[i];
        const packet::seqnum_t sn2 = packet::seqnum_t(sn1 + MaxSnJump + 1);

        packet::Queue queue;
        Validator validator(queue, config, SampleSpecs);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, sn2, 1, 0);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, sn1, 2, 0);
        queue.write(p2);
        CHECK(!validator.read());

        CHECK(!queue.read());
    }
}

TEST(validator, seqnum_late) {
    const packet::seqnum_t sn1 = 100;
    const packet::seqnum_t sn2 = 50;
    const packet::seqnum_t sn3 = sn2 + MaxSnJump + 1;

    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, sn1, 1, 0);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, sn2, 2, 0);
    queue.write(p2);
    CHECK(validator.read() == p2);

    packet::PacketPtr p3 = new_packet(Pt1, Src1, sn3, 3, 0);
    queue.write(p3);
    CHECK(validator.read() == p3);

    CHECK(!queue.read());
}

TEST(validator, timestamp_no_jump) {
    const packet::stream_timestamp_t tss[] = {
        1,
        packet::stream_timestamp_t(-1) - MaxTsJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::stream_timestamp_t ts1 = tss[i];
        const packet::stream_timestamp_t ts2 = ts1 + MaxTsJump;

        packet::Queue queue;
        Validator validator(queue, config, SampleSpecs);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, ts1, 0);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, ts2, 0);
        queue.write(p2);
        CHECK(validator.read() == p2);

        CHECK(!queue.read());
    }
}

TEST(validator, timestamp_jump_up) {
    const packet::stream_timestamp_t tss[] = {
        1,
        packet::stream_timestamp_t(-1) - MaxTsJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::stream_timestamp_t ts1 = tss[i];
        const packet::stream_timestamp_t ts2 = ts1 + MaxTsJump + 10;

        packet::Queue queue;
        Validator validator(queue, config, SampleSpecs);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, ts1, 0);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, ts2, 0);
        queue.write(p2);
        CHECK(!validator.read());

        CHECK(!queue.read());
    }
}

TEST(validator, timestamp_jump_down) {
    const packet::stream_timestamp_t tss[] = {
        1,
        packet::stream_timestamp_t(-1) - MaxTsJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::stream_timestamp_t ts1 = tss[i];
        const packet::stream_timestamp_t ts2 = ts1 + MaxTsJump + 10;

        packet::Queue queue;
        Validator validator(queue, config, SampleSpecs);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, ts2, 0);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, ts1, 0);
        queue.write(p2);
        CHECK(!validator.read());

        CHECK(!queue.read());
    }
}

TEST(validator, timestamp_late) {
    const packet::stream_timestamp_t ts1 = 100;
    const packet::stream_timestamp_t ts2 = 50;
    const packet::stream_timestamp_t ts3 = ts2 + MaxTsJump + 1;

    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 2, ts1, 0);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, 1, ts2, 0);
    queue.write(p2);
    CHECK(validator.read() == p2);

    packet::PacketPtr p3 = new_packet(Pt1, Src1, 3, ts3, 0);
    queue.write(p3);
    CHECK(validator.read() == p3);

    CHECK(!queue.read());
}

TEST(validator, cts_positive) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1, 100);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, 2, 50);
    queue.write(p2);
    CHECK(validator.read() == p2);

    packet::PacketPtr p3 = new_packet(Pt1, Src1, 3, 3, 200);
    queue.write(p3);
    CHECK(validator.read() == p3);

    packet::PacketPtr p4 = new_packet(Pt1, Src1, 4, 4, 150);
    queue.write(p4);
    CHECK(validator.read() == p4);

    CHECK(!queue.read());
}

TEST(validator, cts_negative) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1, 100);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, 2, -100);
    queue.write(p2);
    CHECK(!validator.read());

    packet::PacketPtr p3 = new_packet(Pt1, Src1, 3, 3, 200);
    queue.write(p3);
    CHECK(validator.read() == p3);

    packet::PacketPtr p4 = new_packet(Pt1, Src1, 4, 4, -200);
    queue.write(p4);
    CHECK(!validator.read());

    CHECK(!queue.read());
}

TEST(validator, cts_zero) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1, 100);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, 2, 0);
    queue.write(p2);
    CHECK(!validator.read());

    packet::PacketPtr p3 = new_packet(Pt1, Src1, 3, 3, 200);
    queue.write(p3);
    CHECK(validator.read() == p3);

    packet::PacketPtr p4 = new_packet(Pt1, Src1, 4, 4, 0);
    queue.write(p4);
    CHECK(!validator.read());

    CHECK(!queue.read());
}

} // namespace rtp
} // namespace roc
