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
#include "roc_pipeline/transcoder_source.h"

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

TEST_GROUP(transcoder_source) {
    audio::SampleSpec input_sample_spec;
    audio::SampleSpec output_sample_spec;

    TranscoderConfig make_config() {
        TranscoderConfig config;

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

TEST(transcoder_source, state) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    test::MockSource mock_source;

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    mock_source.set_state(sndio::DeviceState_Active);
    CHECK(transcoder.state() == sndio::DeviceState_Active);

    mock_source.set_state(sndio::DeviceState_Idle);
    CHECK(transcoder.state() == sndio::DeviceState_Idle);
}

TEST(transcoder_source, pause_resume) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    test::MockSource mock_source;

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    transcoder.pause();
    CHECK(transcoder.state() == sndio::DeviceState_Paused);
    CHECK(mock_source.state() == sndio::DeviceState_Paused);

    CHECK(transcoder.resume());
    CHECK(transcoder.state() == sndio::DeviceState_Active);
    CHECK(mock_source.state() == sndio::DeviceState_Active);
}

TEST(transcoder_source, pause_restart) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    test::MockSource mock_source;

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    transcoder.pause();
    CHECK(transcoder.state() == sndio::DeviceState_Paused);
    CHECK(mock_source.state() == sndio::DeviceState_Paused);

    CHECK(transcoder.restart());
    CHECK(transcoder.state() == sndio::DeviceState_Active);
    CHECK(mock_source.state() == sndio::DeviceState_Active);
}

TEST(transcoder_source, read) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    test::FrameReader frame_reader(transcoder, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, eof) {
    enum { NumCh = 2 };

    init(NumCh, NumCh);

    test::MockSource mock_source;

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    core::Slice<audio::sample_t> samples = sample_buffer_factory.new_buffer();
    samples.reslice(0, SamplesPerFrame * NumCh);

    audio::Frame frame(samples.data(), samples.size());

    mock_source.add(SamplesPerFrame, input_sample_spec);
    CHECK(transcoder.read(frame));
    CHECK(!transcoder.read(frame));
}

TEST(transcoder_source, frame_size_small) {
    enum { NumCh = 2, SamplesPerSmallFrame = SamplesPerFrame / 2 - 3 };

    init(NumCh, NumCh);

    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerSmallFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    test::FrameReader frame_reader(transcoder, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerSmallFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, frame_size_large) {
    enum { NumCh = 2, SamplesPerLargeFrame = SamplesPerFrame * 2 + 3 };

    init(NumCh, NumCh);

    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerLargeFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    test::FrameReader frame_reader(transcoder, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerLargeFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, channels_stereo_to_mono) {
    enum { InputCh = 2, OutputCh = 1 };

    init(InputCh, OutputCh);

    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    test::FrameReader frame_reader(transcoder, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, channels_mono_to_stereo) {
    enum { InputCh = 1, OutputCh = 2 };

    init(InputCh, OutputCh);

    test::MockSource mock_source;
    mock_source.add(ManyFrames * SamplesPerFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, sample_buffer_factory,
                                allocator);
    CHECK(transcoder.is_valid());

    test::FrameReader frame_reader(transcoder, sample_buffer_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

} // namespace pipeline
} // namespace roc
