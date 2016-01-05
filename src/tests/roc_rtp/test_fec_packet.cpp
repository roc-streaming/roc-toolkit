/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_rtp/parser.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace test {

using namespace packet;

TEST_GROUP(fec_packet) {
    enum { PayloadSz = 77 };

    rtp::Composer composer;
    rtp::Parser parser;

    IFECPacketPtr compose() {
        IPacketPtr packet = composer.compose(IFECPacket::Type);
        CHECK(packet);
        CHECK(packet->type() == IFECPacket::Type);
        return static_cast<IFECPacket*>(packet.get());
    }

    IFECPacketConstPtr parse(const core::IByteBufferConstSlice& buff) {
        IPacketConstPtr packet = parser.parse(buff);
        CHECK(packet);
        CHECK(packet->type() == IFECPacket::Type);
        return static_cast<const IFECPacket*>(packet.get());
    }

    void set_payload(const IFECPacketPtr& packet) {
        uint8_t data[PayloadSz] = {};
        for (size_t n = 0; n < PayloadSz; n++) {
            data[n] = (uint8_t)n;
        }
        packet->set_payload(data, PayloadSz);
    }

    void check_payload(const IFECPacketConstPtr& packet) {
        core::IByteBufferConstSlice buff = packet->payload();
        CHECK(buff);
        LONGS_EQUAL(PayloadSz, buff.size());
        for (size_t n = 0; n < PayloadSz; n++) {
            LONGS_EQUAL(n, buff.data()[n]);
        }
    }
};

TEST(fec_packet, compose_empty) {
    IFECPacketPtr p = compose();

    LONGS_EQUAL(0, p->source());
    LONGS_EQUAL(0, p->seqnum());

    CHECK(!p->marker());

    LONGS_EQUAL(0, p->data_blknum());
    LONGS_EQUAL(0, p->fec_blknum());

    p->set_payload(NULL, 0);
    CHECK(!p->payload());
}

TEST(fec_packet, compose_full) {
    IFECPacketPtr p = compose();

    p->set_source(1122334455);
    p->set_seqnum(12345);
    p->set_marker(true);

    p->set_data_blknum(54321);
    p->set_fec_blknum(44444);

    LONGS_EQUAL(1122334455, p->source());
    LONGS_EQUAL(12345, p->seqnum());
    CHECK(p->marker());

    LONGS_EQUAL(54321, p->data_blknum());
    LONGS_EQUAL(44444, p->fec_blknum());

    set_payload(p);
    check_payload(p);
}

TEST(fec_packet, compose_parse) {
    IFECPacketPtr p1 = compose();

    p1->set_source(1122334455);
    p1->set_seqnum(12345);
    p1->set_marker(true);

    p1->set_data_blknum(54321);
    p1->set_fec_blknum(44444);

    set_payload(p1);

    IFECPacketConstPtr p2 = parse(p1->raw_data());

    LONGS_EQUAL(1122334455, p2->source());
    LONGS_EQUAL(12345, p2->seqnum());
    CHECK(p2->marker());

    LONGS_EQUAL(54321, p2->data_blknum());
    LONGS_EQUAL(44444, p2->fec_blknum());

    check_payload(p2);
}

} // namespace test
} // namespace roc
