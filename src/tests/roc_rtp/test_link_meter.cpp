/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_defs.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/link_meter.h"

namespace roc {
namespace rtp {

namespace {

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
EncodingMap encoding_map(arena);

packet::PacketPtr new_packet(packet::seqnum_t sn) {
    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(packet::Packet::FlagRTP);
    packet->rtp()->payload_type = PayloadType_L16_Stereo;
    packet->rtp()->seqnum = sn;

    return packet;
}

class StatusWriter : public packet::IWriter {
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

TEST_GROUP(link_meter) {};

TEST(link_meter, has_metrics) {
    packet::Queue queue;
    LinkMeter meter(encoding_map);
    meter.set_writer(queue);

    CHECK(!meter.has_metrics());

    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(100)));
    UNSIGNED_LONGS_EQUAL(1, queue.size());

    CHECK(meter.has_metrics());
}

TEST(link_meter, last_seqnum) {
    packet::Queue queue;
    LinkMeter meter(encoding_map);
    meter.set_writer(queue);

    UNSIGNED_LONGS_EQUAL(0, meter.metrics().ext_last_seqnum);

    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(100)));
    UNSIGNED_LONGS_EQUAL(100, meter.metrics().ext_last_seqnum);

    // seqnum increased, metric updated
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(102)));
    UNSIGNED_LONGS_EQUAL(102, meter.metrics().ext_last_seqnum);

    // seqnum decreased, ignored
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(101)));
    UNSIGNED_LONGS_EQUAL(102, meter.metrics().ext_last_seqnum);

    // seqnum increased, metric updated
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(103)));
    UNSIGNED_LONGS_EQUAL(103, meter.metrics().ext_last_seqnum);

    UNSIGNED_LONGS_EQUAL(4, queue.size());
}

TEST(link_meter, last_seqnum_wrap) {
    packet::Queue queue;
    LinkMeter meter(encoding_map);
    meter.set_writer(queue);

    UNSIGNED_LONGS_EQUAL(0, meter.metrics().ext_last_seqnum);

    // no overflow
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(65533)));
    UNSIGNED_LONGS_EQUAL(65533, meter.metrics().ext_last_seqnum);

    // no overflow
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(65535)));
    UNSIGNED_LONGS_EQUAL(65535, meter.metrics().ext_last_seqnum);

    // overflow
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(2)));
    UNSIGNED_LONGS_EQUAL(65537, meter.metrics().ext_last_seqnum);

    // late packet, ignored
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(65534)));
    UNSIGNED_LONGS_EQUAL(65537, meter.metrics().ext_last_seqnum);

    // new packet
    LONGS_EQUAL(status::StatusOK, meter.write(new_packet(5)));
    UNSIGNED_LONGS_EQUAL(65540, meter.metrics().ext_last_seqnum);

    UNSIGNED_LONGS_EQUAL(5, queue.size());
}

TEST(link_meter, forward_error) {
    StatusWriter writer(status::StatusNoMem);
    LinkMeter meter(encoding_map);
    meter.set_writer(writer);

    LONGS_EQUAL(status::StatusNoMem, meter.write(new_packet(100)));
}

} // namespace rtp
} // namespace roc
