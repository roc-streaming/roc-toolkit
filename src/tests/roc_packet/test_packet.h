/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PACKET_TEST_PACKET_H_
#define ROC_PACKET_TEST_PACKET_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_pool.h"
#include "roc_packet/iaudio_packet.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace test {

static inline packet::IAudioPacketPtr new_packet(packet::seqnum_t sn,
                                                 packet::timestamp_t ts = 0) {
    static rtp::Composer composer;

    packet::IPacketPtr packet = composer.compose(packet::IAudioPacket::Type);
    CHECK(packet);

    packet::IAudioPacketPtr audio = static_cast<packet::IAudioPacket*>(packet.get());

    audio->set_seqnum(sn);
    audio->set_timestamp(ts);

    return audio;
}

} // namespace test
} // namespace roc

#endif // ROC_PACKET_TEST_PACKET_H_
