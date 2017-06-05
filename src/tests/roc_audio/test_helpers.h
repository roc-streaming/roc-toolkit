/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_H_
#define ROC_AUDIO_TEST_HELPERS_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/math.h"

#include "roc_packet/ipacket.h"
#include "roc_rtp/composer.h"

#include "roc_audio/istream_reader.h"
#include "roc_audio/sample_buffer.h"

namespace roc {
namespace test {

template <size_t BufSz> static inline audio::ISampleBufferPtr new_buffer(size_t sz) {
    audio::ISampleBufferPtr buf =
        audio::SampleBufferTraits::default_composer<BufSz>().compose();

    CHECK(buf);

    buf->set_size(sz);

    for (size_t n = 0; n < sz; n++) {
        buf->data()[n] = 9999;
    }

    return buf;
}

inline packet::IPacketPtr new_audio_packet() {
    static rtp::Composer composer;

    packet::IPacketPtr packet = composer.compose(packet::IPacket::HasAudio);
    CHECK(packet);
    CHECK(packet->audio());

    return packet;
}

static inline void
expect_data(const packet::sample_t* buf, size_t bufsz, packet::sample_t value) {
    size_t num_bad_samples = 0;

    for (size_t n = 0; n < bufsz; n++) {
        if (ROC_ABS(value - buf[n]) > 0.0001f) {
            num_bad_samples++;
        }
    }

    LONGS_EQUAL(0, num_bad_samples);
}

template <size_t BufSz>
static inline void read_buffers(audio::IStreamReader& reader,
                                size_t num_buffers,
                                size_t sz,
                                packet::sample_t value) {
    for (size_t n = 0; n < num_buffers; n++) {
        audio::ISampleBufferPtr buf = new_buffer<BufSz>(sz);

        reader.read(*buf);

        LONGS_EQUAL(sz, buf->size());

        expect_data(buf->data(), sz, value);
    }
}

} // namespace test
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_H_
