/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_MOCK_SINK_H_
#define ROC_PIPELINE_TEST_HELPERS_MOCK_SINK_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/noncopyable.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {
namespace test {

class MockSink : public sndio::ISink, public core::NonCopyable<> {
public:
    MockSink(const audio::SampleSpec& sample_spec, core::IArena& arena)
        : sndio::IDevice(arena)
        , sndio::ISink(arena)
        , off_(0)
        , n_frames_(0)
        , n_samples_(0)
        , n_chans_(sample_spec.num_channels()) {
    }

    virtual sndio::DeviceType type() const {
        return sndio::DeviceType_Sink;
    }

    virtual sndio::ISink* to_sink() {
        return this;
    }

    virtual sndio::ISource* to_source() {
        return NULL;
    }

    virtual audio::SampleSpec sample_spec() const {
        return audio::SampleSpec();
    }

    core::nanoseconds_t frame_length() const {
        return 0;
    }

    virtual bool has_state() const {
        return false;
    }

    virtual bool has_latency() const {
        return false;
    }

    virtual bool has_clock() const {
        return false;
    }

    virtual status::StatusCode write(audio::Frame& frame) {
        CHECK(frame.num_raw_samples() % n_chans_ == 0);

        for (size_t ns = 0; ns < frame.num_raw_samples() / n_chans_; ns++) {
            for (size_t nc = 0; nc < n_chans_; nc++) {
                DOUBLES_EQUAL((double)frame.raw_samples()[ns * n_chans_ + nc],
                              (double)nth_sample(off_), SampleEpsilon);
                n_samples_++;
            }
            off_++;
        }
        n_frames_++;

        CHECK(frame.capture_timestamp() == 0);

        return status::StatusOK;
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode flush() {
        return status::StatusOK;
    }

    virtual status::StatusCode close() {
        return status::StatusOK;
    }

    virtual void dispose() {
        arena().dispose_object(*this);
    }

    void expect_frames(size_t total) {
        UNSIGNED_LONGS_EQUAL(total, n_frames_);
    }

    void expect_samples(size_t total) {
        UNSIGNED_LONGS_EQUAL(total * n_chans_, n_samples_);
    }

private:
    uint8_t off_;

    size_t n_frames_;
    size_t n_samples_;
    size_t n_chans_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_MOCK_SINK_H_
