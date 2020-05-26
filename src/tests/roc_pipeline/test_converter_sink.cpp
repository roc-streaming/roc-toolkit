/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_checker.h"
#include "test_helpers/frame_writer.h"

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_pipeline/converter_sink.h"

namespace roc {
namespace pipeline {

namespace {

enum {
    MaxBufSize = 1000,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

    SamplesPerFrame = 20,
    ManyFrames = 30
};

const core::nanoseconds_t MaxBufDuration =
    MaxBufSize * core::Second / (SampleRate * packet::num_channels(ChMask));

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, true);

} // namespace

TEST_GROUP(converter_sink) {
    ConverterConfig config;

    void setup() {
        config.input_channels = ChMask;
        config.output_channels = ChMask;

        config.input_sample_rate = SampleRate;
        config.output_sample_rate = SampleRate;
        config.internal_frame_length = MaxBufDuration;

        config.resampling = false;
        config.poisoning = true;
        config.profiling = true;
    }
};

TEST(converter_sink, null) {
    ConverterSink converter(config, NULL, sample_buffer_pool, allocator);
    CHECK(converter.valid());

    test::FrameWriter frame_writer(converter, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * NumCh);
    }
}

TEST(converter_sink, write) {
    test::FrameChecker frame_checker;

    ConverterSink converter(config, &frame_checker, sample_buffer_pool, allocator);
    CHECK(converter.valid());

    test::FrameWriter frame_writer(converter, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * NumCh);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerFrame * NumCh);
}

TEST(converter_sink, frame_size_small) {
    enum { SamplesPerSmallFrame = SamplesPerFrame / 2 - 3 };

    test::FrameChecker frame_checker;

    ConverterSink converter(config, &frame_checker, sample_buffer_pool, allocator);
    CHECK(converter.valid());

    test::FrameWriter frame_writer(converter, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame * NumCh);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerSmallFrame * NumCh);
}

TEST(converter_sink, frame_size_large) {
    enum { SamplesPerLargeFrame = SamplesPerFrame * 2 + 3 };

    test::FrameChecker frame_checker;

    ConverterSink converter(config, &frame_checker, sample_buffer_pool, allocator);
    CHECK(converter.valid());

    test::FrameWriter frame_writer(converter, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame * NumCh);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerLargeFrame * NumCh);
}

} // namespace pipeline
} // namespace roc
