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

template <size_t MaxPackets, size_t NumSamples, int ChNum, int ChMask>
class TestAudioPacketReader : public audio::IAudioPacketReader {
public:
    enum { Rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE };

    TestAudioPacketReader()
        : pos_(0)
        , max_(0) {
    }

    ~TestAudioPacketReader() {
        LONGS_EQUAL(max_, pos_);
    }

    virtual packet::IAudioPacketConstPtr read(packet::channel_t ch) {
        LONGS_EQUAL(ChNum, ch);

        if (pos_ == max_) {
            return NULL;
        } else {
            return packets_[pos_++];
        }
    }

    void add(packet::timestamp_t timestamp, packet::sample_t value) {
        CHECK(max_ != MaxPackets);

        packet::IAudioPacketPtr packet = new_audio_packet();

        packet::sample_t samples[NumSamples];

        for (size_t n = 0; n < NumSamples; n++) {
            samples[n] = value;
        }

        packet->set_timestamp(timestamp);
        packet->set_size(ChMask, NumSamples, Rate);
        packet->write_samples((1 << ChNum), 0, samples, NumSamples);

        packets_[max_++] = packet;
    }

private:
    packet::IAudioPacketPtr packets_[MaxPackets];
    size_t pos_;
    size_t max_;
};

} // namespace test
} // namespace roc

#endif // ROC_AUDIO_TEST_AUDIO_PACKET_READER_H_
