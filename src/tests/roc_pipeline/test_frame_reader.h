/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_FRAME_READER_H_
#define ROC_PIPELINE_TEST_FRAME_READER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_pipeline/ireceiver.h"

#include "test_helpers.h"

namespace roc {
namespace pipeline {

class FrameReader : public core::NonCopyable<> {
public:
    FrameReader(IReceiver& receiver, core::BufferPool<audio::sample_t>& pool)
        : receiver_(receiver)
        , pool_(pool)
        , offset_(0) {
    }

    void read_samples(size_t num_samples, size_t num_sessions) {
        audio::Frame frame;
        frame.samples = new (pool_) core::Buffer<audio::sample_t>(pool_);
        frame.samples.resize(num_samples);

        receiver_.read(frame);

        UNSIGNED_LONGS_EQUAL(num_samples, frame.samples.size());

        for (size_t n = 0; n < num_samples; n++) {
            DOUBLES_EQUAL(nth_sample(offset_) * num_sessions, frame.samples.data()[n],
                          Epsilon);
            offset_++;
        }
    }

    void skip_zeros(size_t num_samples) {
        audio::Frame frame;
        frame.samples = new (pool_) core::Buffer<audio::sample_t>(pool_);
        frame.samples.resize(num_samples);

        memset(frame.samples.data(), 0, frame.samples.size() * sizeof(audio::sample_t));

        receiver_.read(frame);

        UNSIGNED_LONGS_EQUAL(num_samples, frame.samples.size());

        for (size_t n = 0; n < num_samples; n++) {
            CHECK(frame.samples.data()[n] == 0.0f);
        }
    }

private:
    IReceiver& receiver_;
    core::BufferPool<audio::sample_t>& pool_;

    uint8_t offset_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_FRAME_READER_H_
