/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_FRAME_READER_H_
#define ROC_PIPELINE_TEST_HELPERS_FRAME_READER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {
namespace test {

class FrameReader : public core::NonCopyable<> {
public:
    FrameReader(sndio::ISource& source,
                core::BufferFactory<audio::sample_t>& buffer_factory)
        : source_(source)
        , buffer_factory_(buffer_factory)
        , offset_(0) {
    }

    void read_samples(size_t num_samples,
                      size_t num_sessions,
                      const audio::SampleSpec& sample_spec) {
        core::Slice<audio::sample_t> samples = buffer_factory_.new_buffer();
        CHECK(samples);
        samples.reslice(0, num_samples * sample_spec.num_channels());

        audio::Frame frame(samples.data(), samples.size());
        CHECK(source_.read(frame));

        for (size_t ns = 0; ns < num_samples; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                DOUBLES_EQUAL(
                    (double)nth_sample(offset_) * num_sessions,
                    (double)frame.samples()[ns * sample_spec.num_channels() + nc],
                    Epsilon);
            }
            offset_++;
        }
    }

    void skip_zeros(size_t num_samples, const audio::SampleSpec& sample_spec) {
        core::Slice<audio::sample_t> samples = buffer_factory_.new_buffer();
        CHECK(samples);

        samples.reslice(0, num_samples * sample_spec.num_channels());
        memset(samples.data(), 0, samples.size() * sizeof(audio::sample_t));

        audio::Frame frame(samples.data(), samples.size());
        CHECK(source_.read(frame));

        for (size_t n = 0; n < num_samples * sample_spec.num_channels(); n++) {
            DOUBLES_EQUAL(0.0, (double)frame.samples()[n], Epsilon);
        }
    }

    void set_offset(size_t offset) {
        offset_ = uint8_t(offset);
    }

private:
    sndio::ISource& source_;
    core::BufferFactory<audio::sample_t>& buffer_factory_;

    uint8_t offset_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_FRAME_READER_H_
