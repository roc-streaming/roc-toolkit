/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_
#define ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/iframe_reader.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

class MockReader : public IFrameReader {
public:
    explicit MockReader(bool fail_on_empty = true)
        : pos_(0)
        , size_(0)
        , fail_on_empty_(fail_on_empty) {
    }

    virtual bool read(Frame& frame) {
        if (fail_on_empty_) {
            CHECK(pos_ + frame.num_samples() <= size_);
        } else if (pos_ + frame.num_samples() > size_) {
            return false;
        }

        memcpy(frame.samples(), samples_ + pos_, frame.num_samples() * sizeof(sample_t));

        unsigned flags = 0;
        for (size_t n = pos_; n < pos_ + frame.num_samples(); n++) {
            flags |= flags_[n];
        }
        frame.set_flags(flags);

        pos_ += frame.num_samples();

        return true;
    }

    void add(size_t size, sample_t value, unsigned flags = 0) {
        CHECK(size_ + size < MaxSz);

        for (size_t n = 0; n < size; n++) {
            samples_[size_] = value;
            flags_[size_] = flags;
            size_++;
        }
    }

    void pad_zeros() {
        while (size_ < MaxSz) {
            samples_[size_] = 0;
            flags_[size_] = 0;
            size_++;
        }
    }

    size_t num_unread() const {
        return size_ - pos_;
    }

private:
    enum { MaxSz = 64 * 1024 };

    sample_t samples_[MaxSz];
    unsigned flags_[MaxSz];
    size_t pos_;
    size_t size_;
    const bool fail_on_empty_;
};

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_
