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

#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
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

const audio::ChannelMask Chans_Mono = audio::ChanMask_Surround_Mono;
const audio::ChannelMask Chans_Stereo = audio::ChanMask_Surround_Stereo;

core::HeapArena arena;

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer>
    frame_buffer_pool("frame_buffer_pool",
                      arena,
                      sizeof(core::Buffer) + MaxBufSize * sizeof(audio::sample_t));

audio::FrameFactory frame_factory(frame_pool, frame_buffer_pool);

audio::ProcessorMap processor_map(arena);

void read_frame(status::StatusCode expected_code,
                audio::IFrameReader& reader,
                size_t samples_per_chan) {
    audio::FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    const status::StatusCode code =
        reader.read(*frame, samples_per_chan, audio::ModeHard);

    LONGS_EQUAL(expected_code, code);
}

} // namespace

TEST_GROUP(transcoder_source) {
    audio::SampleSpec input_sample_spec;
    audio::SampleSpec output_sample_spec;

    TranscoderConfig make_config() {
        TranscoderConfig config;

        config.input_sample_spec = input_sample_spec;
        config.output_sample_spec = output_sample_spec;

        config.enable_profiling = true;

        return config;
    }

    void init(audio::ChannelMask input_channels, audio::ChannelMask output_channels) {
        input_sample_spec.set_format(audio::Format_Pcm);
        input_sample_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
        input_sample_spec.set_sample_rate(SampleRate);
        input_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        input_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        input_sample_spec.channel_set().set_mask(input_channels);

        output_sample_spec.set_format(audio::Format_Pcm);
        output_sample_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
        output_sample_spec.set_sample_rate(SampleRate);
        output_sample_spec.channel_set().set_layout(audio::ChanLayout_Surround);
        output_sample_spec.channel_set().set_order(audio::ChanOrder_Smpte);
        output_sample_spec.channel_set().set_mask(output_channels);
    }
};

TEST(transcoder_source, state) {
    enum { Chans = Chans_Stereo };

    init(Chans, Chans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    mock_source.set_state(sndio::DeviceState_Active);
    CHECK(transcoder.state() == sndio::DeviceState_Active);

    mock_source.set_state(sndio::DeviceState_Idle);
    CHECK(transcoder.state() == sndio::DeviceState_Idle);
}

TEST(transcoder_source, pause_resume) {
    enum { Chans = Chans_Stereo };

    init(Chans, Chans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    LONGS_EQUAL(status::StatusOK, transcoder.pause());
    CHECK(transcoder.state() == sndio::DeviceState_Paused);
    CHECK(mock_source.state() == sndio::DeviceState_Paused);

    LONGS_EQUAL(status::StatusOK, transcoder.resume());
    CHECK(transcoder.state() == sndio::DeviceState_Active);
    CHECK(mock_source.state() == sndio::DeviceState_Active);
}

TEST(transcoder_source, pause_restart) {
    enum { Chans = Chans_Stereo };

    init(Chans, Chans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    LONGS_EQUAL(status::StatusOK, transcoder.pause());
    CHECK(transcoder.state() == sndio::DeviceState_Paused);
    CHECK(mock_source.state() == sndio::DeviceState_Paused);

    LONGS_EQUAL(status::StatusOK, transcoder.rewind());
    CHECK(transcoder.state() == sndio::DeviceState_Active);
    CHECK(mock_source.state() == sndio::DeviceState_Active);
}

TEST(transcoder_source, read) {
    enum { Chans = Chans_Stereo };

    init(Chans, Chans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);
    mock_source.add(ManyFrames * SamplesPerFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    test::FrameReader frame_reader(transcoder, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, eof) {
    enum { Chans = Chans_Stereo };

    init(Chans, Chans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    mock_source.add(SamplesPerFrame, input_sample_spec);

    read_frame(status::StatusOK, transcoder, SamplesPerFrame);
    read_frame(status::StatusFinish, transcoder, SamplesPerFrame);
}

TEST(transcoder_source, frame_size_small) {
    enum { Chans = Chans_Stereo, SamplesPerSmallFrame = SamplesPerFrame / 2 - 3 };

    init(Chans, Chans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);
    mock_source.add(ManyFrames * SamplesPerSmallFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    test::FrameReader frame_reader(transcoder, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerSmallFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, frame_size_large) {
    enum { Chans = Chans_Stereo, SamplesPerLargeFrame = SamplesPerFrame * 2 + 3 };

    init(Chans, Chans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);
    mock_source.add(ManyFrames * SamplesPerLargeFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    test::FrameReader frame_reader(transcoder, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerLargeFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, channel_mapping_stereo_to_mono) {
    enum { InputChans = Chans_Stereo, OutputChans = Chans_Mono };

    init(InputChans, OutputChans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);
    mock_source.add(ManyFrames * SamplesPerFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    test::FrameReader frame_reader(transcoder, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

TEST(transcoder_source, channel_mapping_mono_to_stereo) {
    enum { InputChans = Chans_Mono, OutputChans = Chans_Stereo };

    init(InputChans, OutputChans);

    test::MockSource mock_source(frame_factory, input_sample_spec, arena);
    mock_source.add(ManyFrames * SamplesPerFrame, input_sample_spec);

    TranscoderSource transcoder(make_config(), mock_source, processor_map, frame_pool,
                                frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, transcoder.init_status());

    test::FrameReader frame_reader(transcoder, frame_factory);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_reader.read_samples(SamplesPerFrame, 1, output_sample_spec);
    }

    UNSIGNED_LONGS_EQUAL(mock_source.num_remaining(), 0);
}

} // namespace pipeline
} // namespace roc
