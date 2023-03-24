/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_reader.h"
#include "test_helpers/mock_source.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_allocator.h"
#include "roc_pipeline/converter_source.h"

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

const audio::SampleSpec SampleSpecs = audio::SampleSpec(SampleRate, ChMask);

const core::nanoseconds_t MaxBufDuration = MaxBufSize * core::Second
    / core::nanoseconds_t(SampleSpecs.sample_rate() * SampleSpecs.num_channels());

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);

} // namespace

TEST_GROUP(converter_source) {
    ConverterConfig config;

    void setup() {
        config.input_sample_spec = audio::SampleSpec(SampleRate, ChMask);
        config.output_sample_spec = audio::SampleSpec(SampleRate, ChMask);

        config.internal_frame_length = MaxBufDuration;

        config.resampling = false;
        config.poisoning = true;
        config.profiling = true;
    }
};

TEST(converter_source, state) {
    test::MockSource mock_source;

    ConverterSource converter(config, mock_source, sample_buffer_factory, allocator);
    CHECK(converter.valid());

    mock_source.set_state(sndio::DeviceState_Active);
    CHECK(converter.state() == sndio::DeviceState_Active);

    mock_source.set_state(sndio::DeviceState_Idle);
    CHECK(converter.state() == sndio::DeviceState_Idle);
}

TEST(converter_source, pause_resume) {
    test::MockSource mock_source;

    ConverterSource converter(config, mock_source, sample_buffer_factory, allocator);
    CHECK(converter.valid());

    converter.pause();
    CHECK(converter.state() == sndio::DeviceState_Paused);
    CHECK(mock_source.state() == sndio::DeviceState_Paused);

    CHECK(converter.resume());
    CHECK(converter.state() == sndio::DeviceState_Active);
    CHECK(mock_source.state() == sndio::DeviceState_Active);
}

TEST(converter_source, pause_restart) {
    test::MockSource mock_source;

    ConverterSource converter(config, mock_source, sample_buffer_factory, allocator);
    CHECK(converter.valid());

    converter.pause();
    CHECK(converter.state() == sndio::DeviceState_Paused);
    CHECK(mock_source.state() == sndio::DeviceState_Paused);

    CHECK(converter.restart());
    CHECK(converter.state() == sndio::DeviceState_Active);
    CHECK(mock_source.state() == sndio::DeviceState_Active);
}

TEST(converter_source, read) {
    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerFrame * NumCh);

    ConverterSource converter(config, mock_source, sample_buffer_factory, allocator);
    CHECK(converter.valid());

    test::FrameReader frame_reader(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(converter_source, eof) {
    test::MockSource mock_source;

    ConverterSource converter(config, mock_source, sample_buffer_factory, allocator);
    CHECK(converter.valid());

    core::Slice<audio::sample_t> samples = sample_buffer_factory.new_buffer();
    samples.reslice(0, SamplesPerFrame * NumCh);

    audio::Frame frame(samples.data(), samples.size());

    mock_source.add(SamplesPerFrame * NumCh);
    CHECK(converter.read(frame));
    CHECK(!converter.read(frame));
}

TEST(converter_source, frame_size_small) {
    enum { SamplesPerSmallFrame = SamplesPerFrame / 2 - 3 };

    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerSmallFrame * NumCh);

    ConverterSource converter(config, mock_source, sample_buffer_factory, allocator);
    CHECK(converter.valid());

    test::FrameReader frame_reader(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerSmallFrame * NumCh, 1);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(converter_source, frame_size_large) {
    enum { SamplesPerLargeFrame = SamplesPerFrame * 2 + 3 };

    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerLargeFrame * NumCh);

    ConverterSource converter(config, mock_source, sample_buffer_factory, allocator);
    CHECK(converter.valid());

    test::FrameReader frame_reader(converter, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerLargeFrame * NumCh, 1);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

} // namespace pipeline
} // namespace roc
