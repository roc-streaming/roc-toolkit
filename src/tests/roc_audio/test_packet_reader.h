/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_PACKET_READER_H_
#define ROC_AUDIO_TEST_PACKET_READER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/panic.h"
#include "roc_packet/ipacket_reader.h"

#include "test_helpers.h"

namespace roc {
namespace test {

template <size_t MaxPackets> class TestPacketReader : public packet::IPacketReader {
public:
    TestPacketReader()
        : pos_(0)
        , max_(0) {
    }

    virtual packet::IPacketConstPtr read() {
        if (pos_ == max_) {
            return NULL;
        } else {
            return packets_[pos_++];
        }
    }

    void rewind() {
        pos_ = 0;
    }

    void add(const packet::IPacketPtr& packet = new_audio_packet()) {
        CHECK(max_ != MaxPackets);
        packets_[max_++] = packet;
    }

    size_t num_returned() const {
        return pos_;
    }

    void expect_returned(size_t n, const packet::IPacketConstPtr& expected) const {
        CHECK(n < pos_);
        CHECK(packets_[n] == expected);
    }

private:
    packet::IPacketPtr packets_[MaxPackets];
    size_t pos_;
    size_t max_;
};

} // namespace test
} // namespace roc

#endif // ROC_AUDIO_TEST_PACKET_READER_H_
