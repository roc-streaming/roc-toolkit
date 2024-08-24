/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/status_reader.h"

#include "roc_audio/pcm_decoder.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtp/filter.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

namespace {

const PayloadType Pt1 = PayloadType_L16_Stereo;
const PayloadType Pt2 = PayloadType_L16_Mono;

enum {
    Src1 = 55,
    Src2 = 77,
    SampleRate = 10000,
    ChMask = 3,
    PacketSz = 128,
    MaxSnJump = 100,
    MaxTsJump = 1000
};

const audio::SampleSpec payload_spec(SampleRate,
                                     audio::PcmSubformat_SInt16_Be,
                                     audio::ChanLayout_Surround,
                                     audio::ChanOrder_Smpte,
                                     ChMask);

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, PacketSz);

audio::PcmDecoder decoder(payload_spec, arena);

packet::PacketPtr new_packet(PayloadType pt,
                             packet::stream_source_t src,
                             packet::seqnum_t sn,
                             packet::stream_timestamp_t ts,
                             core::nanoseconds_t cts = 0,
                             packet::stream_timestamp_t duration = 0,
                             unsigned flags = (packet::Packet::FlagRTP
                                               | packet::Packet::FlagAudio)) {
    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(flags);

    if (packet->has_flags(packet::Packet::FlagRTP)) {
        packet->rtp()->payload_type = pt;
        packet->rtp()->source_id = src;
        packet->rtp()->seqnum = sn;
        packet->rtp()->stream_timestamp = ts;
        packet->rtp()->capture_timestamp = cts;
        packet->rtp()->duration = duration;

        core::Slice<uint8_t> buffer = packet_factory.new_packet_buffer();
        CHECK(buffer);
        packet->rtp()->payload = buffer;
    }

    return packet;
}

void write_packet(packet::IWriter& writer, const packet::PacketPtr& pp) {
    CHECK(pp);
    LONGS_EQUAL(status::StatusOK, writer.write(pp));
}

packet::PacketPtr expect_read(status::StatusCode expect_code,
                              packet::IReader& reader,
                              packet::PacketReadMode mode) {
    packet::PacketPtr pp;
    LONGS_EQUAL(expect_code, reader.read(pp, mode));
    if (expect_code == status::StatusOK) {
        CHECK(pp);
    } else {
        CHECK(!pp);
    }
    return pp;
}

} // namespace

TEST_GROUP(filter) {
    FilterConfig config;

    void setup() {
        memset((void*)&config, 0, sizeof(config));
        config.max_sn_jump = MaxSnJump;
        config.max_ts_jump = MaxTsJump * core::Second / SampleRate;
    }
};

TEST(filter, all_good) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, payload_id_jump) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt2, Src1, 2, 2);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, source_id_jump) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src2, 2, 2);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, seqnum_no_jump) {
    const packet::seqnum_t sn_list[] = {
        1,
        packet::seqnum_t(-1) - MaxSnJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(sn_list); i++) {
        const packet::seqnum_t sn1 = sn_list[i];
        const packet::seqnum_t sn2 = packet::seqnum_t(sn1 + MaxSnJump);

        packet::FifoQueue queue;
        Filter filter(queue, decoder, config, payload_spec);
        LONGS_EQUAL(status::StatusOK, filter.init_status());

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 1);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 2);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        LONGS_EQUAL(0, queue.size());
    }
}

TEST(filter, seqnum_jump_up) {
    const packet::seqnum_t sn_list[] = {
        1,
        packet::seqnum_t(-1) - MaxSnJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(sn_list); i++) {
        const packet::seqnum_t sn1 = sn_list[i];
        const packet::seqnum_t sn2 = packet::seqnum_t(sn1 + MaxSnJump + 1);

        packet::FifoQueue queue;
        Filter filter(queue, decoder, config, payload_spec);
        LONGS_EQUAL(status::StatusOK, filter.init_status());

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 1);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 2);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusDrain, filter, packet::ModeFetch);
            CHECK(!rp);
        }

        LONGS_EQUAL(0, queue.size());
    }
}

TEST(filter, seqnum_jump_down) {
    const packet::seqnum_t sn_list[] = {
        1,
        packet::seqnum_t(-1) - MaxSnJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(sn_list); i++) {
        const packet::seqnum_t sn1 = sn_list[i];
        const packet::seqnum_t sn2 = packet::seqnum_t(sn1 + MaxSnJump + 1);

        packet::FifoQueue queue;
        Filter filter(queue, decoder, config, payload_spec);
        LONGS_EQUAL(status::StatusOK, filter.init_status());

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 1);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 2);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusDrain, filter, packet::ModeFetch);
            CHECK(!rp);
        }

        LONGS_EQUAL(0, queue.size());
    }
}

TEST(filter, seqnum_late) {
    const packet::seqnum_t sn1 = 100;
    const packet::seqnum_t sn2 = 50;
    const packet::seqnum_t sn3 = sn2 + MaxSnJump + 1;

    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 1);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 2);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, sn3, 3);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, timestamp_no_jump) {
    const packet::stream_timestamp_t ts_list[] = {
        1,
        packet::stream_timestamp_t(-1) - MaxTsJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(ts_list); i++) {
        const packet::stream_timestamp_t ts1 = ts_list[i];
        const packet::stream_timestamp_t ts2 = ts1 + MaxTsJump;

        packet::FifoQueue queue;
        Filter filter(queue, decoder, config, payload_spec);
        LONGS_EQUAL(status::StatusOK, filter.init_status());

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts1);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts2);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        LONGS_EQUAL(0, queue.size());
    }
}

TEST(filter, timestamp_jump_up) {
    const packet::stream_timestamp_t ts_list[] = {
        1,
        packet::stream_timestamp_t(-1) - MaxTsJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(ts_list); i++) {
        const packet::stream_timestamp_t ts1 = ts_list[i];
        const packet::stream_timestamp_t ts2 = ts1 + MaxTsJump + 10;

        packet::FifoQueue queue;
        Filter filter(queue, decoder, config, payload_spec);
        LONGS_EQUAL(status::StatusOK, filter.init_status());

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts1);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts2);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusDrain, filter, packet::ModeFetch);
            CHECK(!rp);
        }

        LONGS_EQUAL(0, queue.size());
    }
}

TEST(filter, timestamp_jump_down) {
    const packet::stream_timestamp_t ts_list[] = {
        1,
        packet::stream_timestamp_t(-1) - MaxTsJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(ts_list); i++) {
        const packet::stream_timestamp_t ts1 = ts_list[i];
        const packet::stream_timestamp_t ts2 = ts1 + MaxTsJump + 10;

        packet::FifoQueue queue;
        Filter filter(queue, decoder, config, payload_spec);
        LONGS_EQUAL(status::StatusOK, filter.init_status());

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts2);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModeFetch);
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts1);
            write_packet(queue, wp);

            packet::PacketPtr rp =
                expect_read(status::StatusDrain, filter, packet::ModeFetch);
            CHECK(!rp);
        }

        LONGS_EQUAL(0, queue.size());
    }
}

TEST(filter, timestamp_late) {
    const packet::stream_timestamp_t ts1 = 100;
    const packet::stream_timestamp_t ts2 = 50;
    const packet::stream_timestamp_t ts3 = ts2 + MaxTsJump + 1;

    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts1);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts2);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, ts3);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, cts_positive) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1, 100);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2, 50);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, 3, 200);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 4, 4, 150);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, cts_negative) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1, 100);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2, -100);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, 3, 200);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 4, 4, -200);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, cts_zero) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1, 100);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2, 0);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, 3, 200);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 4, 4, 0);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, duration_zero) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    const packet::stream_timestamp_t packet_duration = 0;
    const packet::stream_timestamp_t expected_duration = 32;

    packet::PacketPtr wp = new_packet(Pt1, Src1, 0, 0, 0, packet_duration);
    write_packet(queue, wp);

    packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
    CHECK(rp);
    CHECK(wp == rp);

    UNSIGNED_LONGS_EQUAL(expected_duration, rp->rtp()->duration);
}

TEST(filter, duration_non_zero) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    const packet::stream_timestamp_t duration = 100;

    packet::PacketPtr wp = new_packet(Pt1, Src1, 0, 0, 0, duration);
    write_packet(queue, wp);

    packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
    CHECK(rp);
    CHECK(wp == rp);

    UNSIGNED_LONGS_EQUAL(duration, rp->rtp()->duration);
}

TEST(filter, flags) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    {
        packet::PacketPtr wp =
            new_packet(Pt1, Src1, 1, 1, 100, 1, packet::Packet::FlagRTP);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp =
            new_packet(Pt1, Src1, 1, 1, 100, 1, packet::Packet::FlagAudio);
        write_packet(queue, wp);

        packet::PacketPtr rp =
            expect_read(status::StatusDrain, filter, packet::ModeFetch);
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp = new_packet(
            Pt1, Src1, 1, 1, 100, 1, packet::Packet::FlagRTP | packet::Packet::FlagAudio);
        write_packet(queue, wp);

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(wp == rp);
    }

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, skip_invalid) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    { // 3 bad packets
        write_packet(queue, new_packet(Pt1, Src1, 1, 1, 1, 1, 0));
        write_packet(queue, new_packet(Pt1, Src1, 2, 2, 2, 2, 0));
        write_packet(queue, new_packet(Pt1, Src1, 3, 3, 3, 3, 0));
    }

    // 1 good packet
    packet::PacketPtr wp = new_packet(
        Pt1, Src1, 4, 4, 4, 4, packet::Packet::FlagRTP | packet::Packet::FlagAudio);
    write_packet(queue, wp);

    UNSIGNED_LONGS_EQUAL(4, queue.size());

    // read: skip all bad and return good
    packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
    CHECK(wp == rp);

    LONGS_EQUAL(0, queue.size());
}

TEST(filter, fetch_peek) {
    packet::FifoQueue queue;
    Filter filter(queue, decoder, config, payload_spec);
    LONGS_EQUAL(status::StatusOK, filter.init_status());

    // 1st good packet
    packet::PacketPtr wp1 = new_packet(
        Pt1, Src1, 4, 4, 4, 4, packet::Packet::FlagRTP | packet::Packet::FlagAudio);
    write_packet(queue, wp1);

    { // 3 bad packets
        write_packet(queue, new_packet(Pt1, Src1, 1, 1, 1, 1, 0));
        write_packet(queue, new_packet(Pt1, Src1, 2, 2, 2, 2, 0));
        write_packet(queue, new_packet(Pt1, Src1, 3, 3, 3, 3, 0));
    }

    // 2nd good packet
    packet::PacketPtr wp2 = new_packet(
        Pt1, Src1, 5, 5, 5, 5, packet::Packet::FlagRTP | packet::Packet::FlagAudio);
    write_packet(queue, wp2);

    {
        for (int i = 0; i < 5; i++) {
            packet::PacketPtr rp =
                expect_read(status::StatusOK, filter, packet::ModePeek);
            CHECK(rp == wp1);
        }

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(rp == wp1);
    }

    {
        for (int i = 0; i < 5; i++) {
            packet::PacketPtr rp =
                expect_read(status::StatusDrain, filter, packet::ModePeek);
            CHECK(!rp);
        }

        packet::PacketPtr rp = expect_read(status::StatusOK, filter, packet::ModeFetch);
        CHECK(rp == wp2);
    }
}

TEST(filter, forward_error) {
    const status::StatusCode status_list[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        test::StatusReader reader(status_list[st_n]);
        Filter filter(reader, decoder, config, payload_spec);
        LONGS_EQUAL(status::StatusOK, filter.init_status());

        expect_read(status_list[st_n], filter, packet::ModePeek);
        expect_read(status_list[st_n], filter, packet::ModeFetch);
    }
}

} // namespace rtp
} // namespace roc
