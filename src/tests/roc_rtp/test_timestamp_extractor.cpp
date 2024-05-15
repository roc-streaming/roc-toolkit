/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_packet/units.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/parser.h"
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

class StatusWriter : public packet::IWriter, public core::NonCopyable<> {
public:
    explicit StatusWriter(status::StatusCode code)
        : code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr&) {
        return code_;
    }

private:
    status::StatusCode code_;
};

} // namespace

TEST_GROUP(timestamp_extractor) {};

TEST(timestamp_extractor, single_write) {
    // 1 second = 1000 samples
    const audio::SampleSpec sample_spec =
        audio::SampleSpec(1000, audio::Sample_RawFormat, audio::ChanLayout_Surround,
                          audio::ChanOrder_Smpte, 0x1);

    const core::nanoseconds_t cts = 1691499037871419405;
    const packet::stream_timestamp_t rts = 2222;

    packet::Queue queue;
    TimestampExtractor extractor(queue, sample_spec);

    // no mapping yet
    CHECK_FALSE(extractor.has_mapping());

    // write packet
    packet::PacketPtr wp = new_packet(555, rts, cts);
    LONGS_EQUAL(status::StatusOK, extractor.write(wp));

    // ensure packet was passed to inner writer
    UNSIGNED_LONGS_EQUAL(1, queue.size());
    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, queue.read(rp));
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

TEST(timestamp_extractor, failed_to_write_packet) {
    // 1 second = 1000 samples
    const audio::SampleSpec sample_spec =
        audio::SampleSpec(1000, audio::Sample_RawFormat, audio::ChanLayout_Surround,
                          audio::ChanOrder_Smpte, 0x1);

    const status::StatusCode codes[] = {
        status::StatusUnknown,
        status::StatusNoData,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(codes); ++n) {
        StatusWriter writer(codes[n]);
        TimestampExtractor extractor(writer, sample_spec);

        packet::PacketPtr pp = new_packet(555, 0, 0);
        LONGS_EQUAL(codes[n], extractor.write(pp));
    }
}

} // namespace rtp
} // namespace roc
