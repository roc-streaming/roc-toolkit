/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/status_writer.h"

#include "roc_core/heap_arena.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtp/timestamp_extractor.h"

namespace roc {
namespace rtp {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, MaxBufSize);

packet::PacketPtr new_packet(packet::seqnum_t sn,
                             packet::stream_timestamp_t ts,
                             core::nanoseconds_t capt_ts) {
    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(packet::Packet::FlagRTP);
    packet->rtp()->seqnum = sn;
    packet->rtp()->stream_timestamp = ts;
    packet->rtp()->capture_timestamp = capt_ts;

    return packet;
}

} // namespace

TEST_GROUP(timestamp_extractor) {};

TEST(timestamp_extractor, single_write) {
    // 1 second = 1000 samples
    const audio::SampleSpec sample_spec =
        audio::SampleSpec(1000, audio::PcmSubformat_Raw, audio::ChanLayout_Surround,
                          audio::ChanOrder_Smpte, 0x1);

    const core::nanoseconds_t cts = 1691499037871419405;
    const packet::stream_timestamp_t rts = 2222;

    packet::FifoQueue queue;
    TimestampExtractor extractor(queue, sample_spec);

    // no mapping yet
    CHECK_FALSE(extractor.has_mapping());

    // write packet
    packet::PacketPtr wp = new_packet(555, rts, cts);
    LONGS_EQUAL(status::StatusOK, extractor.write(wp));

    // ensure packet was passed to inner writer
    UNSIGNED_LONGS_EQUAL(1, queue.size());
    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, queue.read(rp, packet::ModeFetch));
    CHECK_EQUAL(wp, rp);

    // get mapping for exact time
    CHECK_TRUE(extractor.has_mapping());
    UNSIGNED_LONGS_EQUAL(rts, extractor.get_mapping(cts));

    // get mapping for time in future
    CHECK_TRUE(extractor.has_mapping());
    UNSIGNED_LONGS_EQUAL(rts + 1000, extractor.get_mapping(cts + core::Second));

    // get mapping for time in past
    CHECK_TRUE(extractor.has_mapping());
    UNSIGNED_LONGS_EQUAL(rts - 1000, extractor.get_mapping(cts - core::Second));
}

TEST(timestamp_extractor, forward_error) {
    // 1 second = 1000 samples
    const audio::SampleSpec sample_spec =
        audio::SampleSpec(1000, audio::PcmSubformat_Raw, audio::ChanLayout_Surround,
                          audio::ChanOrder_Smpte, 0x1);

    const status::StatusCode status_list[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        test::StatusWriter writer(status_list[st_n]);
        TimestampExtractor extractor(writer, sample_spec);

        packet::PacketPtr pp = new_packet(555, 0, 0);
        LONGS_EQUAL(status_list[st_n], extractor.write(pp));
    }
}

} // namespace rtp
} // namespace roc
