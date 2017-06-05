/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/resampler.h"
#include "roc_config/config.h"
#include "roc_core/scoped_ptr.h"

#include "test_stream_reader.h"

namespace roc {
namespace test {

using namespace audio;

namespace {

enum { FrameSize = ROC_CONFIG_DEFAULT_RESAMPLER_FRAME_SAMPLES * 2 - 1 };

enum { OutSamples = FrameSize * 100 + 1, InSamples = OutSamples + (FrameSize * 3) };

} // namespace

TEST_GROUP(resampler) {
    TestStreamReader<InSamples> reader;

    core::ScopedPtr<Resampler> resampler;

    void setup() {
        resampler.reset(new Resampler(reader, default_buffer_composer(), FrameSize));
    }

    void expect_buffers(size_t num_buffers, size_t sz, int value) {
        read_buffers<InSamples>(*resampler, num_buffers, sz, value);
    }
};

TEST(resampler, invalid_scaling) {
    enum { InvalidScaling = FrameSize };

    CHECK(!resampler->set_scaling(InvalidScaling));
}

TEST(resampler, no_scaling_one_read) {
    CHECK(resampler->set_scaling(1.0f));

    reader.add(InSamples, 333);

    expect_buffers(1, OutSamples, 333);
}

TEST(resampler, no_scaling_multiple_reads) {
    CHECK(resampler->set_scaling(1.0f));

    for (int n = 0; n < InSamples; n++) {
        reader.add(1, n);
    }

    for (int n = 0; n < OutSamples; n++) {
        expect_buffers(1, 1, FrameSize + n);
    }
}

} // namespace test
} // namespace roc
