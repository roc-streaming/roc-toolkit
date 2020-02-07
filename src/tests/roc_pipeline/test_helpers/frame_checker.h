/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_FRAME_CHECKER_H_
#define ROC_PIPELINE_TEST_HELPERS_FRAME_CHECKER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {
namespace test {

class FrameChecker : public sndio::ISink, public core::NonCopyable<> {
public:
    FrameChecker()
        : off_(0)
        , n_frames_(0)
        , n_samples_(0) {
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

    virtual void write(audio::Frame& frame) {
        for (size_t n = 0; n < frame.size(); n++) {
            DOUBLES_EQUAL((double)frame.data()[n], (double)nth_sample(off_), Epsilon);
            off_++;
            n_samples_++;
        }
        n_frames_++;
    }

    void expect_frames(size_t total) {
        UNSIGNED_LONGS_EQUAL(total, n_frames_);
    }

    void expect_samples(size_t total) {
        UNSIGNED_LONGS_EQUAL(total, n_samples_);
    }

private:
    uint8_t off_;

    size_t n_frames_;
    size_t n_samples_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_FRAME_CHECKER_H_
