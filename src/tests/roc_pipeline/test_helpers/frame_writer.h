/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_
#define ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {
namespace test {

class FrameWriter : public core::NonCopyable<> {
public:
    FrameWriter(sndio::ISink& sink, core::BufferPool<audio::sample_t>& pool)
        : sink_(sink)
        , pool_(pool)
        , offset_(0) {
    }

    void write_samples(size_t num_samples) {
        core::Slice<audio::sample_t> samples(new (pool_)
                                                 core::Buffer<audio::sample_t>(pool_));
        CHECK(samples);
        samples.resize(num_samples);

        for (size_t n = 0; n < num_samples; n++) {
            samples.data()[n] = nth_sample(offset_);
            offset_++;
        }

        audio::Frame frame(samples.data(), samples.size());
        sink_.write(frame);
    }

private:
    sndio::ISink& sink_;
    core::BufferPool<audio::sample_t>& pool_;

    uint8_t offset_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_
