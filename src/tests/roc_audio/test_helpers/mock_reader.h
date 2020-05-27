/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_
#define ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/ireader.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

class MockReader : public IReader {
public:
    MockReader()
        : pos_(0)
        , size_(0) {
    }

    virtual bool read(Frame& frame) {
        CHECK(pos_ + frame.size() <= size_);

        memcpy(frame.data(), samples_ + pos_, frame.size() * sizeof(sample_t));
        pos_ += frame.size();

        return true;
    }

    void add(size_t size, sample_t value) {
        CHECK(size_ + size < MaxSz);

        for (size_t n = 0; n < size; n++) {
            samples_[size_++] = value;
        }
    }

    void pad_zeros() {
        while (size_ < MaxSz) {
            samples_[size_++] = 0;
        }
    }

    size_t num_unread() const {
        return size_ - pos_;
    }

private:
    enum { MaxSz = 64 * 1024 };

    sample_t samples_[MaxSz];
    size_t pos_;
    size_t size_;
};

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_
