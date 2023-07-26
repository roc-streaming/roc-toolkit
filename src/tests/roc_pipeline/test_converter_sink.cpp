/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_checker.h"
#include "test_helpers/frame_writer.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_allocator.h"
#include "roc_pipeline/converter_sink.h"

namespace roc {
namespace pipeline {

namespace {

enum {
    MaxBufSize = 1000,

    SampleRate = 44100,

    SamplesPerFrame = 20,
    ManyFrames = 30
};

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);

} // namespace

TEST_GROUP(converter_sink) {
    audio::SampleSpec input_sample_spec;
    audio::SampleSpec output_sample_spec;

    ConverterConfig make_config() {
        ConverterConfig config;

        config.input_sample_spec = input_sample_spec;
        config.output_sample_spec = output_sample_spec;

        config.enable_poisoning = true;
        config.enable_profiling = true;

        return config;
    }

    void init(size_t input_channels, size_t output_channels) {
        input_sample_spec.set_sample_rate(SampleRate);
        input_sample_spec.channel_set().set_layout(input_channels == 1
                                                       ? audio::ChannelLayout_Mono
                                                       : audio::ChannelLayout_Surround);
        input_sample_spec.channel_set().set_channel_range(0, input_channels - 1, true);

        output_sample_spec.set_sample_rate(SampleRate);
        output_sample_spec.channel_set().set_layout(output_channels == 1
                                                        ? audio::ChannelLayout_Mono
                                                        : audio::ChannelLayout_Surround);
        output_sample_spec.channel_set().set_channel_range(0, output_channels - 1, true);
    }
};

TEST(converter_sink, null) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    ConverterSink converter(make_config(), NULL, sample_buffer_factory, allocator);
    CHECK(converter.is_valid());

    test::FrameWriter frame_writer(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
    }
}

TEST(converter_sink, write) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    test::FrameChecker frame_checker(output_sample_spec);

    ConverterSink converter(make_config(), &frame_checker, sample_buffer_factory,
                            allocator);
    CHECK(converter.is_valid());

    test::FrameWriter frame_writer(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerFrame);
}

TEST(converter_sink, frame_size_small) {
    enum { NumCh = 2, SamplesPerSmallFrame = SamplesPerFrame / 2 - 3 };

    init(NumCh, NumCh);

    test::FrameChecker frame_checker(output_sample_spec);

    ConverterSink converter(make_config(), &frame_checker, sample_buffer_factory,
                            allocator);
    CHECK(converter.is_valid());

    test::FrameWriter frame_writer(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame, input_sample_spec);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerSmallFrame);
}

TEST(converter_sink, frame_size_large) {
    enum { NumCh = 2, SamplesPerLargeFrame = SamplesPerFrame * 2 + 3 };

    init(NumCh, NumCh);

    test::FrameChecker frame_checker(output_sample_spec);

    ConverterSink converter(make_config(), &frame_checker, sample_buffer_factory,
                            allocator);
    CHECK(converter.is_valid());

    test::FrameWriter frame_writer(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame, input_sample_spec);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerLargeFrame);
}

TEST(converter_sink, channels_stereo_to_mono) {
    enum { InputCh = 2, OutputCh = 1 };

    init(InputCh, OutputCh);

    test::FrameChecker frame_checker(output_sample_spec);

    ConverterSink converter(make_config(), &frame_checker, sample_buffer_factory,
                            allocator);
    CHECK(converter.is_valid());

    test::FrameWriter frame_writer(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerFrame);
}

TEST(converter_sink, channels_mono_to_stereo) {
    enum { InputCh = 1, OutputCh = 2 };

    init(InputCh, OutputCh);

    test::FrameChecker frame_checker(output_sample_spec);

    ConverterSink converter(make_config(), &frame_checker, sample_buffer_factory,
                            allocator);
    CHECK(converter.is_valid());

    test::FrameWriter frame_writer(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame, input_sample_spec);
    }

    frame_checker.expect_frames(ManyFrames);
    frame_checker.expect_samples(ManyFrames * SamplesPerFrame);
}

} // namespace pipeline
} // namespace roc
