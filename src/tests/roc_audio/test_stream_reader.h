/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_STREAM_READER_H_
#define ROC_AUDIO_TEST_STREAM_READER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/istream_reader.h"

#include "test_helpers.h"

namespace roc {
namespace test {

template <size_t MaxSz> class TestStreamReader : public audio::IStreamReader {
public:
    TestStreamReader()
        : stream_pos_(0) {
        stream_ = new_buffer<MaxSz>(0);
    }

    virtual void read(const audio::ISampleBufferSlice& out) {
        CHECK(stream_pos_ + out.size() <= stream_->size());

        memcpy(out.data(), stream_->data() + stream_pos_,
               out.size() * sizeof(packet::sample_t));

        stream_pos_ += out.size();
    }

    void add(size_t size, int value) {
        size_t oldsz = stream_->size();

        stream_->set_size(oldsz + size);

        for (size_t n = 0; n < size; n++) {
            stream_->data()[oldsz + n] = value;
        }
    }

private:
    audio::ISampleBufferPtr stream_;
    size_t stream_pos_;
};

} // namespace test
} // namespace roc

#endif // ROC_AUDIO_TEST_STREAM_READER_H_
