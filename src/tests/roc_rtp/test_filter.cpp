/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_decoder.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/time.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_rtp/filter.h"
#include "roc_rtp/headers.h"
#include "roc_status/status_code.h"
#include "test_helpers/status_reader.h"

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
                                     audio::PcmFormat_SInt16_Be,
                                     audio::ChanLayout_Surround,
                                     audio::ChanOrder_Smpte,
                                     ChMask);

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, PacketSz);

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

    if (packet->rtp()) {
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
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, payload_id_jump) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt2, Src1, 2, 2);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, source_id_jump) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src2, 2, 2);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, seqnum_no_jump) {
    const packet::seqnum_t sn_list[] = {
        1,
        packet::seqnum_t(-1) - MaxSnJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(sn_list); i++) {
        const packet::seqnum_t sn1 = sn_list[i];
        const packet::seqnum_t sn2 = packet::seqnum_t(sn1 + MaxSnJump);

        packet::Queue queue;
        audio::PcmDecoder decoder(payload_spec);
        Filter filter(queue, decoder, config, payload_spec);

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 1);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 2);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, queue.read(rp));
            CHECK(!rp);
        }
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

        packet::Queue queue;
        audio::PcmDecoder decoder(payload_spec);
        Filter filter(queue, decoder, config, payload_spec);

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 1);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 2);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, filter.read(rp));
            CHECK(!rp);
        }

        {
            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, queue.read(rp));
            CHECK(!rp);
        }
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

        packet::Queue queue;
        audio::PcmDecoder decoder(payload_spec);
        Filter filter(queue, decoder, config, payload_spec);

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 1);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 2);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, filter.read(rp));
            CHECK(!rp);
        }

        {
            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, queue.read(rp));
            CHECK(!rp);
        }
    }
}

TEST(filter, seqnum_late) {
    const packet::seqnum_t sn1 = 100;
    const packet::seqnum_t sn2 = 50;
    const packet::seqnum_t sn3 = sn2 + MaxSnJump + 1;

    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, sn1, 1);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, sn2, 2);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, sn3, 3);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, timestamp_no_jump) {
    const packet::stream_timestamp_t ts_list[] = {
        1,
        packet::stream_timestamp_t(-1) - MaxTsJump / 2,
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(ts_list); i++) {
        const packet::stream_timestamp_t ts1 = ts_list[i];
        const packet::stream_timestamp_t ts2 = ts1 + MaxTsJump;

        packet::Queue queue;
        audio::PcmDecoder decoder(payload_spec);
        Filter filter(queue, decoder, config, payload_spec);

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts1);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts2);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, queue.read(rp));
            CHECK(!rp);
        }
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

        packet::Queue queue;
        audio::PcmDecoder decoder(payload_spec);
        Filter filter(queue, decoder, config, payload_spec);

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts1);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts2);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, filter.read(rp));
            CHECK(!rp);
        }

        {
            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, queue.read(rp));
            CHECK(!rp);
        }
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

        packet::Queue queue;
        audio::PcmDecoder decoder(payload_spec);
        Filter filter(queue, decoder, config, payload_spec);

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts2);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, filter.read(rp));
            CHECK(wp == rp);
        }

        {
            packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts1);
            LONGS_EQUAL(status::StatusOK, queue.write(wp));

            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, filter.read(rp));
            CHECK(!rp);
        }

        {
            packet::PacketPtr rp;
            LONGS_EQUAL(status::StatusNoData, queue.read(rp));
            CHECK(!rp);
        }
    }
}

TEST(filter, timestamp_late) {
    const packet::stream_timestamp_t ts1 = 100;
    const packet::stream_timestamp_t ts2 = 50;
    const packet::stream_timestamp_t ts3 = ts2 + MaxTsJump + 1;

    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, ts1);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, ts2);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, ts3);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, cts_positive) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1, 100);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2, 50);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, 3, 200);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 4, 4, 150);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, cts_negative) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1, 100);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2, -100);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, 3, 200);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 4, 4, -200);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, cts_zero) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 1, 1, 100);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 2, 2, 0);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 3, 3, 200);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }

    {
        packet::PacketPtr wp = new_packet(Pt1, Src1, 4, 4, 0);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, queue.read(rp));
        CHECK(!rp);
    }
}

TEST(filter, duration_zero) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    const packet::stream_timestamp_t packet_duration = 0;
    const packet::stream_timestamp_t expected_duration = 32;

    packet::PacketPtr wp = new_packet(Pt1, Src1, 0, 0, 0, packet_duration);
    LONGS_EQUAL(status::StatusOK, queue.write(wp));

    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, filter.read(rp));
    CHECK(rp);
    CHECK(wp == rp);

    UNSIGNED_LONGS_EQUAL(expected_duration, rp->rtp()->duration);
}

TEST(filter, duration_non_zero) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    const packet::stream_timestamp_t duration = 100;

    packet::PacketPtr wp = new_packet(Pt1, Src1, 0, 0, 0, duration);
    LONGS_EQUAL(status::StatusOK, queue.write(wp));

    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, filter.read(rp));
    CHECK(rp);
    CHECK(wp == rp);

    UNSIGNED_LONGS_EQUAL(duration, rp->rtp()->duration);
}

TEST(filter, flags) {
    packet::Queue queue;
    audio::PcmDecoder decoder(payload_spec);
    Filter filter(queue, decoder, config, payload_spec);

    {
        packet::PacketPtr wp =
            new_packet(Pt1, Src1, 1, 1, 100, 1, packet::Packet::FlagRTP);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp =
            new_packet(Pt1, Src1, 1, 1, 100, 1, packet::Packet::FlagAudio);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusNoData, filter.read(rp));
        CHECK(!rp);
    }

    {
        packet::PacketPtr wp = new_packet(
            Pt1, Src1, 1, 1, 100, 1, packet::Packet::FlagRTP | packet::Packet::FlagAudio);
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        packet::PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, filter.read(rp));
        CHECK(wp == rp);
    }
}

TEST(filter, forward_error) {
    const status::StatusCode code_list[] = {
        status::StatusNoMem,
        status::StatusNoData,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(code_list); ++n) {
        test::StatusReader reader(code_list[n]);
        audio::PcmDecoder decoder(payload_spec);
        Filter filter(reader, decoder, config, payload_spec);

        packet::PacketPtr pp;
        LONGS_EQUAL(code_list[n], filter.read(pp));
        CHECK(!pp);
    }
}

} // namespace rtp
} // namespace roc
