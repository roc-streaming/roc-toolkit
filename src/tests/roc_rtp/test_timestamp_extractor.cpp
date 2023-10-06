/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_packet/units.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"
#include "roc_rtp/timestamp_extractor.h"

namespace roc {
namespace rtp {

namespace {

core::HeapArena arena;
static packet::PacketFactory packet_factory(arena);

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
        audio::SampleSpec(1000, audio::ChanLayout_Surround, 0x1);

    const core::nanoseconds_t cts = 1691499037871419405;
    const packet::stream_timestamp_t rts = 2222;

    packet::Queue queue;
    TimestampExtractor extractor(queue, sample_spec);

    // no mapping yet
    CHECK_FALSE(extractor.has_mapping());

    // write packet
    packet::PacketPtr wp = new_packet(555, rts, cts);
    extractor.write(wp);

    // ensure packet was passed to inner writer
    CHECK_EQUAL(1, queue.size());
    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rp));
    CHECK_EQUAL(wp, rp);

    // get mapping for exact time
    CHECK_TRUE(extractor.has_mapping());
    CHECK_EQUAL(rts, extractor.get_mapping(cts));

    // get mapping for time in future
    CHECK_TRUE(extractor.has_mapping());
    CHECK_EQUAL(rts + 1000, extractor.get_mapping(cts + core::Second));

    // get mapping for time in past
    CHECK_TRUE(extractor.has_mapping());
    CHECK_EQUAL(rts - 1000, extractor.get_mapping(cts - core::Second));
}

} // namespace rtp
} // namespace roc
