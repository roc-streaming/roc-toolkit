/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_PACKET_WRITER_H_
#define ROC_AUDIO_TEST_PACKET_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/panic.h"

#include "roc_packet/ipacket_writer.h"

namespace roc {
namespace test {

template <size_t MaxPackets> class TestPacketWriter : public packet::IPacketWriter {
public:
    virtual void write(const packet::IPacketPtr& pkt) {
        CHECK(pkt);

        packets_.append(pkt);
    }

    size_t num_packets() const {
        return packets_.size();
    }

    packet::IPacketPtr packet(size_t n) {
        CHECK(n < packets_.size());
        return packets_[n];
    }

private:
    core::Array<packet::IPacketPtr, MaxPackets> packets_;
};

} // namespace test
} // namespace roc

#endif // ROC_AUDIO_TEST_PACKET_WRITER_H_
