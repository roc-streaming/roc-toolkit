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

class LastPacketHolder : public packet::IWriter {
public:
    LastPacketHolder()
        : last_pkt_(NULL) {
    }

    virtual ~LastPacketHolder() {
    }

    virtual void write(const packet::PacketPtr& pkt) {
        last_pkt_ = pkt;
    }

    const packet::PacketPtr& get() const {
        return last_pkt_;
    }

private:
    packet::PacketPtr last_pkt_;
};

namespace {

core::HeapArena arena;
static packet::PacketFactory packet_factory(arena);

packet::PacketPtr
new_packet(packet::seqnum_t sn, packet::timestamp_t ts, core::nanoseconds_t capt_ts) {
    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(packet::Packet::FlagRTP);
    packet->rtp()->seqnum = sn;
    packet->rtp()->timestamp = ts;
    packet->rtp()->capture_timestamp = capt_ts;

    return packet;
}
} // namespace

TEST_GROUP(capture_ts_getter) {};

// Basic test.
TEST(capture_ts_getter, single_write) {
    packet::timestamp_t rtp_ts = 2222;
    core::nanoseconds_t cur_packet_capt_ts = 1691499037871419405;

    core::nanoseconds_t cts = cur_packet_capt_ts;
    packet::timestamp_t rts = rtp_ts;

    LastPacketHolder holder;
    TimestampExtractor getter(holder);
    CHECK_FALSE(getter.get_mapping(cts, rts));
    CHECK_EQUAL(cts, cur_packet_capt_ts);
    CHECK_EQUAL(rts, rtp_ts);

    packet::PacketPtr pkt =
        new_packet(555, rtp_ts + 100, cur_packet_capt_ts + core::Second);
    getter.write(pkt);

    CHECK_EQUAL(holder.get(), pkt);

    CHECK(getter.get_mapping(cts, rts));
    CHECK_EQUAL(cts, cur_packet_capt_ts + core::Second);
    CHECK_EQUAL(rts, rtp_ts + 100);
}

} // namespace rtp
} // namespace roc
