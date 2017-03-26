/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_packet/interleaver.h"
#include "roc_packet/packet_queue.h"
#include "roc_core/array.h"
#include "test_packet.h"

namespace roc {
namespace test {

using namespace packet;

TEST_GROUP(interleaver) {
    IPacketPtr new_packet(seqnum_t sn) {
        return new_audio_packet(0, sn, 0);
    }
};

// Fill Interleaver with multiple of its internal memory size.
TEST(interleaver, read_write) {
    enum { MAX_PACKETS = 100 };

    PacketQueue receiver;
    Interleaver intlrvr(receiver, 10);

    const size_t total_packets_num = intlrvr.window_size() * 5;

    // Packets to push to Interleaver.
    core::Array<IPacketPtr, MAX_PACKETS> ppackets(total_packets_num);

    // Checks for received packets.
    core::Array<bool, MAX_PACKETS> packets_ctr(total_packets_num);

    for (size_t i = 0; i < total_packets_num; i++) {
        ppackets[i] = new_packet(seqnum_t(i));
        packets_ctr[i] = false;
    }

    // No packets in interleaver on start.
    LONGS_EQUAL(0, receiver.size());

    // Push every packet to interleaver.
    for (size_t i = 0; i < total_packets_num; i++) {
        intlrvr.write(ppackets[i]);
    }

    // Interleaver must put all packets to its writer because we put pricesly
    // integer number of its window_size.
    LONGS_EQUAL(total_packets_num, receiver.size());

    // Check that packets have different seqnums.
    for (size_t i = 0; i < total_packets_num; i++) {
        IPacketConstPtr p = receiver.read();
        CHECK(p);
        CHECK(p->seqnum() < total_packets_num);
        CHECK(!packets_ctr[p->seqnum()]);
        packets_ctr[p->seqnum()] = true;
    }

    // Nothing left in receiver.
    LONGS_EQUAL(0, receiver.size());
    intlrvr.flush();

    // Nothing left in interleaver.
    LONGS_EQUAL(0, receiver.size());

    // Did we receive all packets that we've sent.
    for (size_t i = 0; i < total_packets_num; i++) {
        CHECK(packets_ctr[i]);
    }
}

TEST(interleaver, flush) {
    PacketQueue receiver;
    Interleaver intlrvr(receiver, 10);

    const size_t total_packets_num = intlrvr.window_size() * 5;

    for (size_t n = 0; n < total_packets_num; n++) {
        IPacketPtr packet = new_packet(seqnum_t(n));

        intlrvr.write(packet);
        LONGS_EQUAL(0, receiver.size());

        intlrvr.flush();
        LONGS_EQUAL(1, receiver.size());

        CHECK(receiver.read() == packet);
        LONGS_EQUAL(0, receiver.size());
    }
}

} // namespace test
} // namespace roc
