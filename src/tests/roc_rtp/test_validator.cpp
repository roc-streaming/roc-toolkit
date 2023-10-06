/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/time.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/config.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/validator.h"
#include "roc_status/status_code.h"
#include "test_helpers/status_reader.h"

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

TEST(validator, failed_to_read_packet) {
    const status::StatusCode codes[] = {
        status::StatusUnknown,
        status::StatusNoData,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(codes); ++n) {
        test::StatusReader reader(codes[n]);
        Validator validator(reader, config, SampleSpecs);

        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(codes[n], validator.read(pp));
        CHECK(!pp);
    }
}

TEST(validator, normal) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, 1, 0);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt1, Src1, 2, 2, 0);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp2));
    CHECK(wp2 == rp2);

    packet::PacketPtr pp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
    CHECK(!pp);
}

TEST(validator, payload_id_jump) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, 1, 0);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt2, Src1, 2, 2, 0);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
    CHECK(!rp2);

    packet::PacketPtr pp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
    CHECK(!pp);
}

TEST(validator, source_id_jump) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, 1, 0);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt1, Src2, 2, 2, 0);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
    CHECK(!rp2);

    packet::PacketPtr pp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
    CHECK(!pp);
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

        packet::PacketPtr wp1 = new_packet(Pt1, Src1, sn1, 1, 0);
        queue.write(wp1);

        packet::PacketPtr rp1;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
        CHECK(wp1 == rp1);

        packet::PacketPtr wp2 = new_packet(Pt1, Src1, sn2, 2, 0);
        queue.write(wp2);

        packet::PacketPtr rp2;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp2));
        CHECK(wp2 == rp2);

        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
        CHECK(!pp);
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

        packet::PacketPtr wp1 = new_packet(Pt1, Src1, sn1, 1, 0);
        queue.write(wp1);

        packet::PacketPtr rp1;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
        CHECK(wp1 == rp1);

        packet::PacketPtr wp2 = new_packet(Pt1, Src1, sn2, 2, 0);
        queue.write(wp2);

        packet::PacketPtr rp2;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
        CHECK(!rp2);

        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
        CHECK(!pp);
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

        packet::PacketPtr wp1 = new_packet(Pt1, Src1, sn2, 1, 0);
        queue.write(wp1);

        packet::PacketPtr rp1;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
        CHECK(wp1 == rp1);

        packet::PacketPtr wp2 = new_packet(Pt1, Src1, sn1, 2, 0);
        queue.write(wp2);

        packet::PacketPtr rp2;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
        CHECK(!rp2);

        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
        CHECK(!pp);
    }
}

TEST(validator, seqnum_late) {
    const packet::seqnum_t sn1 = 100;
    const packet::seqnum_t sn2 = 50;
    const packet::seqnum_t sn3 = sn2 + MaxSnJump + 1;

    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, sn1, 1, 0);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt1, Src1, sn2, 2, 0);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp2));
    CHECK(wp2 == rp2);

    packet::PacketPtr wp3 = new_packet(Pt1, Src1, sn3, 3, 0);
    queue.write(wp3);

    packet::PacketPtr rp3;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp3));
    CHECK(wp3 == rp3);

    packet::PacketPtr pp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
    CHECK(!pp);
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

        packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, ts1, 0);
        queue.write(wp1);

        packet::PacketPtr rp1;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
        CHECK(wp1 == rp1);

        packet::PacketPtr wp2 = new_packet(Pt1, Src1, 2, ts2, 0);
        queue.write(wp2);

        packet::PacketPtr rp2;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp2));
        CHECK(wp2 == rp2);

        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
        CHECK(!pp);
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

        packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, ts1, 0);
        queue.write(wp1);

        packet::PacketPtr rp1;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
        CHECK(wp1 == rp1);

        packet::PacketPtr wp2 = new_packet(Pt1, Src1, 2, ts2, 0);
        queue.write(wp2);

        packet::PacketPtr rp2;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
        CHECK(!rp2);

        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
        CHECK(!pp);
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

        packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, ts2, 0);
        queue.write(wp1);

        packet::PacketPtr rp1;
        UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
        CHECK(wp1 == rp1);

        packet::PacketPtr wp2 = new_packet(Pt1, Src1, 2, ts1, 0);
        queue.write(wp2);

        packet::PacketPtr rp2;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
        CHECK(!rp2);

        packet::PacketPtr pp;
        UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
        CHECK(!pp);
    }
}

TEST(validator, timestamp_late) {
    const packet::stream_timestamp_t ts1 = 100;
    const packet::stream_timestamp_t ts2 = 50;
    const packet::stream_timestamp_t ts3 = ts2 + MaxTsJump + 1;

    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, 2, ts1, 0);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt1, Src1, 1, ts2, 0);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp2));
    CHECK(wp2 == rp2);

    packet::PacketPtr wp3 = new_packet(Pt1, Src1, 3, ts3, 0);
    queue.write(wp3);

    packet::PacketPtr rp3;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp3));
    CHECK(wp3 == rp3);

    packet::PacketPtr pp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
    CHECK(!pp);
}

TEST(validator, cts_positive) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, 1, 100);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt1, Src1, 2, 2, 50);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp2));
    CHECK(wp2 == rp2);

    packet::PacketPtr wp3 = new_packet(Pt1, Src1, 3, 3, 200);
    queue.write(wp3);

    packet::PacketPtr rp3;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp3));
    CHECK(wp3 == rp3);

    packet::PacketPtr wp4 = new_packet(Pt1, Src1, 4, 4, 150);
    queue.write(wp4);

    packet::PacketPtr rp4;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp4));
    CHECK(wp4 == rp4);

    packet::PacketPtr pp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(pp));
    CHECK(!pp);
}

TEST(validator, cts_negative) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, 1, 100);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt1, Src1, 2, 2, -100);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
    CHECK(!rp2);

    packet::PacketPtr wp3 = new_packet(Pt1, Src1, 3, 3, 200);
    queue.write(wp3);

    packet::PacketPtr rp3;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp3));
    CHECK(wp3 == rp3);

    packet::PacketPtr wp4 = new_packet(Pt1, Src1, 4, 4, -200);
    queue.write(wp4);

    packet::PacketPtr rp4;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp4));
    CHECK(!rp4);

    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(rp));
    CHECK(!rp);
}

TEST(validator, cts_zero) {
    packet::Queue queue;
    Validator validator(queue, config, SampleSpecs);

    packet::PacketPtr wp1 = new_packet(Pt1, Src1, 1, 1, 100);
    queue.write(wp1);

    packet::PacketPtr rp1;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp1));
    CHECK(wp1 == rp1);

    packet::PacketPtr wp2 = new_packet(Pt1, Src1, 2, 2, 0);
    queue.write(wp2);

    packet::PacketPtr rp2;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp2));
    CHECK(!rp2);

    packet::PacketPtr wp3 = new_packet(Pt1, Src1, 3, 3, 200);
    queue.write(wp3);

    packet::PacketPtr rp3;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, validator.read(rp3));
    CHECK(wp3 == rp3);

    packet::PacketPtr wp4 = new_packet(Pt1, Src1, 4, 4, 0);
    queue.write(wp4);

    packet::PacketPtr rp4;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, validator.read(rp4));
    CHECK(!rp4);

    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusNoData, queue.read(rp));
    CHECK(!rp);
}

} // namespace rtp
} // namespace roc
