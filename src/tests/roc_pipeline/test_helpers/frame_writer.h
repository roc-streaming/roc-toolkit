/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_
#define ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {
namespace test {

class FrameWriter : public core::NonCopyable<> {
public:
    FrameWriter(sndio::ISink& sink, core::BufferFactory<audio::sample_t>& buffer_factory)
        : sink_(sink)
        , buffer_factory_(buffer_factory)
        , offset_(0) {
    }

    void write_samples(size_t num_samples) {
        core::Slice<audio::sample_t> samples = buffer_factory_.new_buffer();
        CHECK(samples);
        samples.reslice(0, num_samples);

        for (size_t n = 0; n < num_samples; n++) {
            samples.data()[n] = nth_sample(offset_);
            offset_++;
        }

        audio::Frame frame(samples.data(), samples.size());
        sink_.write(frame);
    }

private:
    sndio::ISink& sink_;
    core::BufferFactory<audio::sample_t>& buffer_factory_;

    uint8_t offset_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_
