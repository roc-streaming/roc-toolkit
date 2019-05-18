/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/queue.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/validator.h"

namespace roc {
namespace rtp {

namespace {

const PayloadType Pt1 = PayloadType_L16_Stereo;
const PayloadType Pt2 = PayloadType_L16_Mono;

enum { Src1 = 55, Src2 = 77, SampleRate = 10000, MaxSnJump = 100, MaxTsJump = 1000 };

core::HeapAllocator allocator;
packet::PacketPool pool(allocator, true);

} // namespace

TEST_GROUP(validator) {
    ValidatorConfig config;

    void setup() {
        memset((void*)&config, 0, sizeof(config));
        config.max_sn_jump = MaxSnJump;
        config.max_ts_jump = MaxTsJump * core::Second / SampleRate;
    }

    packet::PacketPtr new_packet(PayloadType pt, packet::source_t src,
                                 packet::seqnum_t sn, packet::timestamp_t ts) {
        packet::PacketPtr packet = new (pool) packet::Packet(pool);
        CHECK(packet);

        packet->add_flags(packet::Packet::FlagRTP);
        packet->rtp()->payload_type = pt;
        packet->rtp()->source = src;
        packet->rtp()->seqnum = sn;
        packet->rtp()->timestamp = ts;

        return packet;
    }
};

TEST(validator, empty) {
    packet::Queue queue;
    Validator validator(queue, config, SampleRate);

    CHECK(!validator.read());
}

TEST(validator, normal) {
    packet::Queue queue;
    Validator validator(queue, config, SampleRate);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, 2);
    queue.write(p2);
    CHECK(validator.read() == p2);

    CHECK(!queue.read());
}

TEST(validator, payload_id_jump) {
    packet::Queue queue;
    Validator validator(queue, config, SampleRate);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt2, Src1, 2, 2);
    queue.write(p2);
    CHECK(!validator.read());

    CHECK(!queue.read());
}

TEST(validator, source_id_jump) {
    packet::Queue queue;
    Validator validator(queue, config, SampleRate);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, 1);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src2, 2, 2);
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
        Validator validator(queue, config, SampleRate);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, sn1, 1);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, sn2, 2);
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
        Validator validator(queue, config, SampleRate);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, sn1, 1);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, sn2, 2);
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
        Validator validator(queue, config, SampleRate);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, sn2, 1);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, sn1, 2);
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
    Validator validator(queue, config, SampleRate);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, sn1, 1);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, sn2, 2);
    queue.write(p2);
    CHECK(validator.read() == p2);

    packet::PacketPtr p3 = new_packet(Pt1, Src1, sn3, 3);
    queue.write(p3);
    CHECK(validator.read() == p3);

    CHECK(!queue.read());
}

TEST(validator, timestamp_no_jump) {
    const packet::timestamp_t tss[] = {
        1,
        packet::timestamp_t(-1) - MaxTsJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::timestamp_t ts1 = tss[i];
        const packet::timestamp_t ts2 = ts1 + MaxTsJump;

        packet::Queue queue;
        Validator validator(queue, config, SampleRate);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, ts1);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, ts2);
        queue.write(p2);
        CHECK(validator.read() == p2);

        CHECK(!queue.read());
    }
}

TEST(validator, timestamp_jump_up) {
    const packet::timestamp_t tss[] = {
        1,
        packet::timestamp_t(-1) - MaxTsJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::timestamp_t ts1 = tss[i];
        const packet::timestamp_t ts2 = ts1 + MaxTsJump + 10;

        packet::Queue queue;
        Validator validator(queue, config, SampleRate);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, ts1);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, ts2);
        queue.write(p2);
        CHECK(!validator.read());

        CHECK(!queue.read());
    }
}

TEST(validator, timestamp_jump_down) {
    const packet::timestamp_t tss[] = {
        1,
        packet::timestamp_t(-1) - MaxTsJump / 2,
    };
    for (size_t i = 0; i < 2; i++) {
        const packet::timestamp_t ts1 = tss[i];
        const packet::timestamp_t ts2 = ts1 + MaxTsJump + 10;

        packet::Queue queue;
        Validator validator(queue, config, SampleRate);

        packet::PacketPtr p1 = new_packet(Pt1, Src1, 1, ts2);
        queue.write(p1);
        CHECK(validator.read() == p1);

        packet::PacketPtr p2 = new_packet(Pt1, Src1, 2, ts1);
        queue.write(p2);
        CHECK(!validator.read());

        CHECK(!queue.read());
    }
}

TEST(validator, timestamp_late) {
    const packet::timestamp_t ts1 = 100;
    const packet::timestamp_t ts2 = 50;
    const packet::timestamp_t ts3 = ts2 + MaxTsJump + 1;

    packet::Queue queue;
    Validator validator(queue, config, SampleRate);

    packet::PacketPtr p1 = new_packet(Pt1, Src1, 2, ts1);
    queue.write(p1);
    CHECK(validator.read() == p1);

    packet::PacketPtr p2 = new_packet(Pt1, Src1, 1, ts2);
    queue.write(p2);
    CHECK(validator.read() == p2);

    packet::PacketPtr p3 = new_packet(Pt1, Src1, 3, ts3);
    queue.write(p3);
    CHECK(validator.read() == p3);

    CHECK(!queue.read());
}

} // namespace rtp
} // namespace roc
