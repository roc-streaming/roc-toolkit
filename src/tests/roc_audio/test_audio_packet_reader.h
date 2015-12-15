/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_AUDIO_PACKET_READER_H_
#define ROC_AUDIO_TEST_AUDIO_PACKET_READER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/iaudio_packet_reader.h"

#include "test_helpers.h"

namespace roc {
namespace test {

template <size_t MAX_PACKETS, size_t NUM_SAMPLES, int CH_NUM, int CH_MASK>
class TestAudioPacketReader : public audio::IAudioPacketReader {
public:
    TestAudioPacketReader()
        : pos_(0)
        , max_(0) {
    }

    ~TestAudioPacketReader() {
        LONGS_EQUAL(max_, pos_);
    }

    virtual packet::IAudioPacketConstPtr read(packet::channel_t ch) {
        LONGS_EQUAL(CH_NUM, ch);

        if (pos_ == max_) {
            return NULL;
        } else {
            return packets_[pos_++];
        }
    }

    void add(packet::timestamp_t timestamp, packet::sample_t value) {
        CHECK(max_ != MAX_PACKETS);

        packet::IAudioPacketPtr packet = new_audio_packet();

        packet::sample_t samples[NUM_SAMPLES];

        for (size_t n = 0; n < NUM_SAMPLES; n++) {
            samples[n] = value;
        }

        packet->set_timestamp(timestamp);
        packet->set_size(CH_MASK, NUM_SAMPLES);
        packet->write_samples((1 << CH_NUM), 0, samples, NUM_SAMPLES);

        packets_[max_++] = packet;
    }

private:
    packet::IAudioPacketPtr packets_[MAX_PACKETS];
    size_t pos_;
    size_t max_;
};

} // namespace test
} // namespace roc

#endif // ROC_AUDIO_TEST_AUDIO_PACKET_READER_H_
