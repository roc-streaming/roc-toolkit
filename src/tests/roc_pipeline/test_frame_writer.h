/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_FRAME_WRITER_H_
#define ROC_PIPELINE_TEST_FRAME_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/iwriter.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"

#include "test_helpers.h"

namespace roc {
namespace pipeline {

class FrameWriter : public core::NonCopyable<> {
public:
    FrameWriter(audio::IWriter& writer, core::BufferPool<audio::sample_t>& pool)
        : writer_(writer)
        , pool_(pool)
        , offset_(0) {
    }

    void write_samples(size_t num_samples) {
        audio::Frame frame;
        frame.samples = new (pool_) core::Buffer<audio::sample_t>(pool_);
        frame.samples.resize(num_samples);

        for (size_t n = 0; n < num_samples; n++) {
            frame.samples.data()[n] = nth_sample(offset_);
            offset_++;
        }

        writer_.write(frame);

        UNSIGNED_LONGS_EQUAL(num_samples, frame.samples.size());
    }

private:
    audio::IWriter& writer_;
    core::BufferPool<audio::sample_t>& pool_;

    uint8_t offset_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_FRAME_WRITER_H_
