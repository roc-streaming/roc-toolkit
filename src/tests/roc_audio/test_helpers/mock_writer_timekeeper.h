/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_MOCK_WRITER_TIMEKEEPER_H_
#define ROC_AUDIO_TEST_HELPERS_MOCK_WRITER_TIMEKEEPER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/iframe_writer.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

class MockWriterTimekeeper : public IFrameWriter {
public:
    explicit MockWriterTimekeeper(const core::nanoseconds_t capt_ts,
                                  const core::nanoseconds_t epsilon,
                                  const audio::SampleSpec& sample_spec)
        : pos_(0)
        , size_(0)
        , capt_ts_(capt_ts)
        , epsilon_(epsilon)
        , sample_spec_(sample_spec)
        , scale_(1.f)
        , start_(true) {
    }

    virtual void write(Frame& frame) {
        CHECK(size_ + frame.num_samples() <= MaxSz);
        if (capt_ts_ && epsilon_) {
            if (start_) {
                start_ = false;
                CHECK(frame.capture_timestamp() >= capt_ts_);
                capt_ts_ = frame.capture_timestamp();
            } else {
                CHECK(
                    core::ns_within_delta(capt_ts_, frame.capture_timestamp(), epsilon_));
            }
            capt_ts_ += core::nanoseconds_t(
                sample_spec_.samples_overall_2_ns(frame.num_samples()) * scale_);
        }

        memcpy(samples_ + size_, frame.samples(), frame.num_samples() * sizeof(sample_t));
        size_ += frame.num_samples();
    }

    sample_t get() {
        CHECK(pos_ < size_);

        return samples_[pos_++];
    }

    size_t num_unread() const {
        return size_ - pos_;
    }

    void set_scaling(const sample_t scale) {
        scale_ = scale;
    }

private:
    enum { MaxSz = 64 * 1024 };

    sample_t samples_[MaxSz];
    size_t pos_;
    size_t size_;
    core::nanoseconds_t capt_ts_;
    core::nanoseconds_t epsilon_;
    const audio::SampleSpec& sample_spec_;
    sample_t scale_;

    bool start_;
};

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_MOCK_WRITER_TIMEKEEPER_H_
