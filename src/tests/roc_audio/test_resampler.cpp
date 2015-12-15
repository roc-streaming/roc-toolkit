/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_config/config.h"
#include "roc_core/scoped_ptr.h"
#include "roc_audio/resampler.h"

#include "test_stream_reader.h"

namespace roc {
namespace test {

using namespace audio;

namespace {

const int FRAME_SIZE = ROC_CONFIG_RESAMPLER_FRAME_SAMPLES;

const float INVALID_SCALING = FRAME_SIZE;

const int TEST_OUTPUT_SAMPLES = FRAME_SIZE * 100 + 1;

const int TEST_INPUT_SAMPLES = TEST_OUTPUT_SAMPLES + (FRAME_SIZE * 3);

} // namespace

TEST_GROUP(resampler) {
    TestStreamReader<TEST_INPUT_SAMPLES> reader;

    core::ScopedPtr<Resampler> resampler;

    void setup() {
        resampler.reset(new Resampler(reader));
    }

    void expect_buffers(size_t num_buffers, size_t sz, int value) {
        read_buffers<TEST_INPUT_SAMPLES>(*resampler, num_buffers, sz, value);
    }
};

TEST(resampler, invalid_scaling) {
    CHECK(!resampler->set_scaling(INVALID_SCALING));
}

TEST(resampler, no_scaling_one_read) {
    CHECK(resampler->set_scaling(1.0f));

    reader.add(TEST_INPUT_SAMPLES, 333);

    expect_buffers(1, TEST_OUTPUT_SAMPLES, 333);
}

TEST(resampler, no_scaling_multiple_reads) {
    CHECK(resampler->set_scaling(1.0f));

    for (int n = 0; n < TEST_INPUT_SAMPLES; n++) {
        reader.add(1, n);
    }

    for (int n = 0; n < TEST_OUTPUT_SAMPLES; n++) {
        expect_buffers(1, 1, FRAME_SIZE + n);
    }
}

} // namespace test
} // namespace roc
