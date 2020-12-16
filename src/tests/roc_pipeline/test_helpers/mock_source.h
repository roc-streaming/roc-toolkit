/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_
#define ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_error/error_code.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {
namespace test {

class MockSource : public sndio::ISource {
public:
    MockSource()
        : state_(Playing)
        , pos_(0)
        , size_(0) {
    }

    virtual size_t sample_rate() const {
        return 0;
    }

    virtual size_t num_channels() const {
        return 0;
    }

    virtual bool has_clock() const {
        return false;
    }

    void set_state(State st) {
        state_ = st;
    }

    virtual State state() const {
        return state_;
    }

    virtual void pause() {
        state_ = Paused;
    }

    virtual bool resume() {
        state_ = Playing;
        return true;
    }

    virtual bool restart() {
        state_ = Playing;
        return true;
    }

    virtual ssize_t read(audio::Frame& frame) {
        if (pos_ == size_) {
            return roc::error::ErrUnknown;
        }

        size_t ns = frame.size();
        if (ns > size_ - pos_) {
            ns = size_ - pos_;
        }

        if (ns > 0) {
            memcpy(frame.data(), samples_ + pos_, ns * sizeof(audio::sample_t));
            pos_ += ns;
        }

        if (ns < frame.size()) {
            memset(frame.data() + ns, 0, (frame.size() - ns) * sizeof(audio::sample_t));
        }

        return ns;
    }

    void add(size_t sz) {
        CHECK(size_ + sz <= MaxSz);

        for (size_t n = 0; n < sz; n++) {
            samples_[size_] = nth_sample((uint8_t)size_);
            size_++;
        }
    }

    size_t num_remaining() const {
        return size_ - pos_;
    }

private:
    enum { MaxSz = 256 * 1024 };

    State state_;

    audio::sample_t samples_[MaxSz];

    size_t pos_;
    size_t size_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_
