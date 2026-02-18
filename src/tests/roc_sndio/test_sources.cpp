/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_sink.h"
#include "test_helpers/mock_source.h"
#include "test_helpers/utils.h"

#include "roc_audio/pcm_mapper_reader.h"
#include "roc_audio/pcm_mapper_writer.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/scoped_ptr.h"
#include "roc_dbgio/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/isink.h"

// This file contains tests for ISource implementations provided by different backends
// in roc_sndio.
//
// Every test iterates through all available backends, and for each one it prepares an
// input file, opens a source, reads frames from it, and retrieved samples.
//
// Test usually defines three sample specs:
//  - file write spec: defines which format/rate/channels to use for prepared
//                     input file
//  - file read spec: tells ISource format/rate/channels to use for input file
//                    (usually only format or format+subformat can be specified)
//  - frame spec: defines which format/rate/channels ISource uses for produced
//                audio frames
//
// In some backends, ISource always produces raw frames and performs conversions from
// file format by itself. In other backends, ISource may produce frames with the
// same format as input file format. This is defined by ISource::sample_spec().

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 500, MaxFrameSize = FrameSize * 10, ManySamples = MaxFrameSize * 10 };

core::HeapArena arena;
audio::FrameFactory frame_factory(arena, MaxFrameSize * sizeof(audio::sample_t));

void write_wav(IBackend& backend,
               const audio::SampleSpec& file_write_spec,
               const audio::SampleSpec& frame_spec,
               const char* path,
               size_t num_samples) {
    CHECK(frame_spec.is_complete());
    CHECK(frame_spec.is_raw());

    const core::nanoseconds_t frame_len = FrameSize * core::Second
        / core::nanoseconds_t(frame_spec.sample_rate() * frame_spec.num_channels());

    test::MockSource mock_source(frame_spec, frame_factory, arena);
    mock_source.add(num_samples * file_write_spec.num_channels());

    IoConfig sink_config;
    sink_config.sample_spec = file_write_spec;
    sink_config.frame_length = frame_len;

    core::ScopedPtr<ISink> sink;
    test::expect_open_sink(status::StatusOK, backend, frame_factory, arena, "file", path,
                           sink_config, sink);

    // sink may consume non-raw frames, so use pcm mapper
    audio::PcmMapperWriter sink_mapper(*sink, frame_factory, frame_spec,
                                       sink->sample_spec());
    LONGS_EQUAL(status::StatusOK, sink_mapper.init_status());

    for (;;) {
        audio::FramePtr frame =
            frame_factory.allocate_frame(frame_spec.ns_2_bytes(frame_len));

        const status::StatusCode code = mock_source.read(
            *frame, frame_spec.ns_2_stream_timestamp(frame_len), audio::ModeHard);

        CHECK(code == status::StatusOK || code == status::StatusPart
              || code == status::StatusFinish);

        if (code == status::StatusFinish) {
            break;
        }

        LONGS_EQUAL(status::StatusOK, sink_mapper.write(*frame));
    }
}

void read_samples(ISource& source,
                  const audio::SampleSpec& frame_spec,
                  size_t num_samples) {
    CHECK(frame_spec.is_complete());
    CHECK(frame_spec.is_raw());

    const core::nanoseconds_t frame_len = FrameSize * core::Second
        / core::nanoseconds_t(frame_spec.sample_rate() * frame_spec.num_channels());

    // source may produce non-raw frames, so use pcm mapper
    audio::PcmMapperReader source_mapper(source, frame_factory, frame_spec,
                                         source.sample_spec());
    LONGS_EQUAL(status::StatusOK, source_mapper.init_status());

    test::MockSink mock_sink(arena);

    for (;;) {
        audio::FramePtr frame =
            frame_factory.allocate_frame(frame_spec.ns_2_bytes(frame_len));

        const status::StatusCode code = source_mapper.read(
            *frame, frame_spec.ns_2_stream_timestamp(frame_len), audio::ModeHard);

        CHECK(code == status::StatusOK || code == status::StatusPart
              || code == status::StatusFinish);

        if (code == status::StatusFinish) {
            break;
        }

        mock_sink.write(*frame);
    }

    mock_sink.check(0, num_samples * frame_spec.num_channels());
}

void expect_read(status::StatusCode expected_code,
                 ISource& source,
                 audio::Frame& frame,
                 packet::stream_timestamp_t requested_samples) {
    const status::StatusCode code =
        source.read(frame, requested_samples, audio::ModeHard);

    LONGS_EQUAL(expected_code, code);
}

audio::ChannelSet make_channel_set(audio::ChannelMask chans) {
    audio::ChannelSet ch_set;
    ch_set.set_layout(audio::ChanLayout_Surround);
    ch_set.set_order(audio::ChanOrder_Smpte);
    ch_set.set_mask(chans);

    return ch_set;
}

IoConfig make_config(const audio::SampleSpec& file_read_spec,
                     const audio::SampleSpec& frame_spec) {
    IoConfig config;
    config.sample_spec = file_read_spec;
    config.frame_length = FrameSize * core::Second
        / core::nanoseconds_t(frame_spec.sample_rate() * frame_spec.num_channels());

    return config;
}

} // namespace

TEST_GROUP(sources) {};

// Don't specify input spec (source will detect everything from file).
TEST(sources, spec_empty) {
    audio::SampleSpec file_read_spec;
    file_read_spec.clear();

    audio::SampleSpec file_write_spec;
    file_write_spec.set_format(audio::Format_Wav);
    file_write_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    file_write_spec.set_sample_rate(44100);
    file_write_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, file_write_spec, frame_spec, file.path(), ManySamples);

        core::ScopedPtr<ISource> source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                                 file.path(), make_config(file_read_spec, frame_spec),
                                 source);

        audio::SampleSpec actual_spec = source->sample_spec();
        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!source->has_state());
        CHECK(!source->has_latency());
        CHECK(!source->has_clock());

        read_samples(*source, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, source->close());
    }
}

// Specify only format (force source to use specific format).
TEST(sources, spec_only_format) {
    audio::SampleSpec file_read_spec;
    file_read_spec.set_format(audio::Format_Wav);

    audio::SampleSpec file_write_spec;
    file_write_spec.set_format(audio::Format_Wav);
    file_write_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    file_write_spec.set_sample_rate(44100);
    file_write_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, file_write_spec, frame_spec, file.path(), ManySamples);

        core::ScopedPtr<ISource> source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                                 file.path(), make_config(file_read_spec, frame_spec),
                                 source);

        audio::SampleSpec actual_spec = source->sample_spec();
        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!source->has_state());
        CHECK(!source->has_latency());
        CHECK(!source->has_clock());

        read_samples(*source, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, source->close());
    }
}

// File with non-default sub-format, rate and channels.
TEST(sources, non_default_input_file) {
    audio::SampleSpec file_read_spec;
    file_read_spec.clear();

    audio::SampleSpec file_write_spec;
    file_write_spec.set_format(audio::Format_Wav);
    file_write_spec.set_pcm_subformat(audio::PcmSubformat_SInt24);
    file_write_spec.set_sample_rate(48000);
    file_write_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Mono));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(48000);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Mono));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, file_write_spec, frame_spec, file.path(), ManySamples);

        core::ScopedPtr<ISource> source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                                 file.path(), make_config(file_read_spec, frame_spec),
                                 source);

        audio::SampleSpec actual_spec = source->sample_spec();
        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!source->has_state());
        CHECK(!source->has_latency());
        CHECK(!source->has_clock());

        read_samples(*source, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, source->close());
    }
}

// File doesn't exist.
TEST(sources, bad_path) {
    audio::SampleSpec file_read_spec;
    file_read_spec.clear();

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        // Expect StatusErrFile, which indicates that there's no need to try other
        // backends (because there is a problem on filesystem level).
        core::ScopedPtr<ISource> source;
        test::expect_open_source(status::StatusErrFile, backend, frame_factory, arena,
                                 "file", "/bad/file.wav",
                                 make_config(file_read_spec, frame_spec), source);
    }
}

// Format not supported by backend.
TEST(sources, bad_format) {
    audio::SampleSpec file_read_spec;
    CHECK(file_read_spec.set_custom_format("bad_fmt"));

    audio::SampleSpec file_write_spec;
    file_write_spec.set_format(audio::Format_Wav);
    file_write_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    file_write_spec.set_sample_rate(44100);
    file_write_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, file_write_spec, frame_spec, file.path(), ManySamples);

        // Expect StatusNoFormat, which indicates that another backend should be tried
        // (another backend may support this format).
        core::ScopedPtr<ISource> source;
        test::expect_open_source(status::StatusNoFormat, backend, frame_factory, arena,
                                 "file", file.path(),
                                 make_config(file_read_spec, frame_spec), source);
    }
}

// Invalid config.
TEST(sources, bad_config) {
    audio::SampleSpec file_read_specs[3];
    // explicit sub-format not allowed
    file_read_specs[0].set_format(audio::Format_Wav);
    file_read_specs[0].set_pcm_subformat(audio::PcmSubformat_Raw);
    // explicit rate not allowed
    file_read_specs[1].set_sample_rate(44100);
    // explicit channels not allowed
    file_read_specs[2].set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    audio::SampleSpec file_write_spec;
    file_write_spec.set_format(audio::Format_Wav);
    file_write_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    file_write_spec.set_sample_rate(44100);
    file_write_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_spec = 0; n_spec < ROC_ARRAY_SIZE(file_read_specs); n_spec++) {
        for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
             n_backend++) {
            IBackend& backend = BackendMap::instance().nth_backend(n_backend);
            if (!test::backend_supports_format(backend, arena, "wav")) {
                continue;
            }

            dbgio::TempFile file("test.wav");
            write_wav(backend, file_write_spec, frame_spec, file.path(), ManySamples);

            // Expect StatusBadConfig, which indicates that there's no need to try other
            // backends (because requested configuration is just invalid).
            core::ScopedPtr<ISource> source;
            test::expect_open_source(
                status::StatusBadConfig, backend, frame_factory, arena, "file",
                file.path(), make_config(file_read_specs[n_spec], frame_spec), source);
        }
    }
}

// Rewind and read same frames again.
TEST(sources, rewind) {
    audio::SampleSpec file_read_spec;
    file_read_spec.clear();

    audio::SampleSpec file_write_spec;
    file_write_spec.set_format(audio::Format_Wav);
    file_write_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    file_write_spec.set_sample_rate(44100);
    file_write_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, file_write_spec, frame_spec, file.path(), ManySamples);

        core::ScopedPtr<ISource> source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                                 file.path(), make_config(file_read_spec, frame_spec),
                                 source);

        // read frame
        audio::FramePtr frame1 = frame_factory.allocate_frame_no_buffer();
        CHECK(frame1);
        expect_read(status::StatusOK, *source, *frame1, FrameSize);

        // rewind
        LONGS_EQUAL(status::StatusOK, source->rewind());

        // read frame again
        audio::FramePtr frame2 = frame_factory.allocate_frame_no_buffer();
        CHECK(frame2);
        expect_read(status::StatusOK, *source, *frame2, FrameSize);

        // compare frames
        LONGS_EQUAL(FrameSize * frame_spec.num_channels(), frame1->num_raw_samples());
        LONGS_EQUAL(FrameSize * frame_spec.num_channels(), frame2->num_raw_samples());

        if (memcmp(frame1->raw_samples(), frame2->raw_samples(),
                   frame1->num_raw_samples() * sizeof(audio::sample_t))
            != 0) {
            FAIL("frames should be equal");
        }

        LONGS_EQUAL(status::StatusOK, source->close());
    }
}

// Read until EOF, rewind, repeat.
TEST(sources, rewind_after_eof) {
    audio::SampleSpec file_read_spec;
    file_read_spec.clear();

    audio::SampleSpec file_write_spec;
    file_write_spec.set_format(audio::Format_Wav);
    file_write_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    file_write_spec.set_sample_rate(44100);
    file_write_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, file_write_spec, frame_spec, file.path(), FrameSize * 2);

        core::ScopedPtr<ISource> source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                                 file.path(), make_config(file_read_spec, frame_spec),
                                 source);

        audio::FramePtr frame = frame_factory.allocate_frame_no_buffer();
        CHECK(frame);

        for (int i = 0; i < 10; i++) {
            expect_read(status::StatusOK, *source, *frame, FrameSize);
            expect_read(status::StatusOK, *source, *frame, FrameSize);
            expect_read(status::StatusFinish, *source, *frame, FrameSize);

            // rewind
            LONGS_EQUAL(status::StatusOK, source->rewind());
        }
        LONGS_EQUAL(status::StatusOK, source->close());
    }
}

} // namespace sndio

} // namespace roc
