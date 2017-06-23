/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_rtp/composer.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace test {

using namespace packet;

TEST_GROUP(fec_packet) {
    enum { PayloadSz = 77 };

    rtp::Composer composer;
    rtp::Parser parser;

    IPacketPtr compose() {
        IPacketPtr packet = composer.compose(IPacket::HasFEC);
        CHECK(packet);
        CHECK(packet->rtp());
        CHECK(packet->fec());
        return packet;
    }

    IPacketConstPtr parse(const core::IByteBufferConstSlice& buff) {
        IPacketConstPtr packet = parser.parse(buff);
        CHECK(packet);
        CHECK(packet->rtp());
        CHECK(packet->fec());
        return packet;
    }

    void set_payload(const IPacketPtr& packet) {
        uint8_t data[PayloadSz] = {};
        for (size_t n = 0; n < PayloadSz; n++) {
            data[n] = (uint8_t)n;
        }
        packet->set_payload(data, PayloadSz);
    }

    void check_payload(const IPacketConstPtr& packet) {
        core::IByteBufferConstSlice buff = packet->payload();
        CHECK(buff);
        LONGS_EQUAL(PayloadSz, buff.size());
        for (size_t n = 0; n < PayloadSz; n++) {
            LONGS_EQUAL(n, buff.data()[n]);
        }
    }
};

TEST(fec_packet, compose_empty) {
    IPacketPtr p = compose();

    LONGS_EQUAL(0, p->rtp()->timestamp());
    LONGS_EQUAL(0, p->rtp()->rate());

    LONGS_EQUAL(0, p->rtp()->source());
    LONGS_EQUAL(0, p->rtp()->seqnum());

    CHECK(!p->rtp()->marker());

    LONGS_EQUAL(0, p->fec()->source_blknum());
    LONGS_EQUAL(0, p->fec()->repair_blknum());

    p->set_payload(NULL, 0);
    CHECK(!p->payload());
}

TEST(fec_packet, compose_full) {
    IPacketPtr p = compose();

    p->rtp()->set_source(1122334455);
    p->rtp()->set_seqnum(12345);
    p->rtp()->set_marker(true);

    p->fec()->set_source_blknum(54321);
    p->fec()->set_repair_blknum(44444);

    LONGS_EQUAL(1122334455, p->rtp()->source());
    LONGS_EQUAL(12345, p->rtp()->seqnum());
    CHECK(p->rtp()->marker());

    LONGS_EQUAL(54321, p->fec()->source_blknum());
    LONGS_EQUAL(44444, p->fec()->repair_blknum());

    set_payload(p);
    check_payload(p);
}

TEST(fec_packet, compose_parse) {
    IPacketPtr p1 = compose();

    p1->rtp()->set_source(1122334455);
    p1->rtp()->set_seqnum(12345);
    p1->rtp()->set_marker(true);

    p1->fec()->set_source_blknum(54321);
    p1->fec()->set_repair_blknum(44444);

    set_payload(p1);

    IPacketConstPtr p2 = parse(p1->raw_data());

    LONGS_EQUAL(1122334455, p2->rtp()->source());
    LONGS_EQUAL(12345, p2->rtp()->seqnum());
    CHECK(p2->rtp()->marker());

    LONGS_EQUAL(54321, p2->fec()->source_blknum());
    LONGS_EQUAL(44444, p2->fec()->repair_blknum());

    check_payload(p2);
}

} // namespace test
} // namespace roc
