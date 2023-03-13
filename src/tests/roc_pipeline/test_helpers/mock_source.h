/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_
#define ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {
namespace test {

class MockSource : public sndio::ISource {
public:
    MockSource()
        : state_(sndio::DeviceState_Active)
        , pos_(0)
        , size_(0) {
    }

    virtual sndio::DeviceType type() const {
        return sndio::DeviceType_Source;
    }

    void set_state(sndio::DeviceState st) {
        state_ = st;
    }

    virtual sndio::DeviceState state() const {
        return state_;
    }

    virtual void pause() {
        state_ = sndio::DeviceState_Paused;
    }

    virtual bool resume() {
        state_ = sndio::DeviceState_Active;
        return true;
    }

    virtual bool restart() {
        state_ = sndio::DeviceState_Active;
        return true;
    }

    virtual audio::SampleSpec sample_spec() const {
        return audio::SampleSpec();
    }

    virtual core::nanoseconds_t latency() const {
        return 0;
    }

    virtual bool has_clock() const {
        return false;
    }

    virtual void reclock(packet::ntp_timestamp_t) {
        // no-op
    }

    virtual bool read(audio::Frame& frame) {
        if (pos_ == size_) {
            return false;
        }

        size_t ns = frame.num_samples();
        if (ns > size_ - pos_) {
            ns = size_ - pos_;
        }

        if (ns > 0) {
            memcpy(frame.samples(), samples_ + pos_, ns * sizeof(audio::sample_t));
            pos_ += ns;
        }

        if (ns < frame.num_samples()) {
            memset(frame.samples() + ns, 0,
                   (frame.num_samples() - ns) * sizeof(audio::sample_t));
        }

        return true;
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

    sndio::DeviceState state_;

    audio::sample_t samples_[MaxSz];

    size_t pos_;
    size_t size_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_
