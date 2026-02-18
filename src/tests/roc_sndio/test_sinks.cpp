/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_sndio/isource.h"

// This file contains tests for ISink implementations provided by different backends
// in roc_sndio.
//
// Every test iterates through all available backends, and for each one it opens a sink
// and writes frames to it. Then it checks that the sink successfully wrote output file.
//
// Test usually defines two sample specs:
//  - file spec: tells ISink format/rate/channels to use for output file
//               (if some parts are omitted, sink will use defaults)
//  - frame spec: defines which format/rate/channels ISink expects to be
//                used for frames written to it
//
// In some backends, ISink always expects raw frames and performs conversions to
// file format by itself. In other backends, ISink may expect frames to have the
// same format as requested output format. This is defined by ISink::sample_spec().

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 500, MaxFrameSize = FrameSize * 10, ManySamples = MaxFrameSize * 10 };

core::HeapArena arena;
audio::FrameFactory frame_factory(arena, MaxFrameSize * sizeof(audio::sample_t));

void write_samples(ISink& sink, const audio::SampleSpec& frame_spec, size_t num_samples) {
    CHECK(frame_spec.is_complete());
    CHECK(frame_spec.is_raw());

    const core::nanoseconds_t frame_len = FrameSize * core::Second
        / core::nanoseconds_t(frame_spec.sample_rate() * frame_spec.num_channels());

    // sink may consume non-raw frames, so use pcm mapper
    audio::PcmMapperWriter sink_mapper(sink, frame_factory, frame_spec,
                                       sink.sample_spec());
    LONGS_EQUAL(status::StatusOK, sink_mapper.init_status());

    test::MockSource mock_source(frame_spec, frame_factory, arena);
    mock_source.add(num_samples * frame_spec.num_channels());

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

void read_wav(IBackend& backend,
              const audio::SampleSpec& frame_spec,
              const char* path,
              size_t num_samples) {
    CHECK(frame_spec.is_complete());
    CHECK(frame_spec.is_raw());

    const core::nanoseconds_t frame_len = FrameSize * core::Second
        / core::nanoseconds_t(frame_spec.sample_rate() * frame_spec.num_channels());

    test::MockSink mock_sink(arena);

    IoConfig source_config;
    source_config.sample_spec = audio::SampleSpec();
    source_config.frame_length = frame_len;

    core::ScopedPtr<ISource> source;
    test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                             path, source_config, source);

    LONGS_EQUAL(frame_spec.sample_rate(), source->sample_spec().sample_rate());
    CHECK(frame_spec.channel_set() == source->sample_spec().channel_set());

    // source may produce non-raw frames, so use pcm mapper
    audio::PcmMapperReader source_mapper(*source, frame_factory, frame_spec,
                                         source->sample_spec());
    LONGS_EQUAL(status::StatusOK, source_mapper.init_status());

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

        LONGS_EQUAL(status::StatusOK, mock_sink.write(*frame));
    }

    mock_sink.check(0, num_samples * frame_spec.num_channels());
}

audio::ChannelSet make_channel_set(audio::ChannelMask chans) {
    audio::ChannelSet ch_set;
    ch_set.set_layout(audio::ChanLayout_Surround);
    ch_set.set_order(audio::ChanOrder_Smpte);
    ch_set.set_mask(chans);

    return ch_set;
}

IoConfig make_config(const audio::SampleSpec& file_spec,
                     const audio::SampleSpec& frame_spec) {
    IoConfig config;
    config.sample_spec = file_spec;
    config.frame_length = FrameSize * core::Second
        / core::nanoseconds_t(frame_spec.sample_rate() * frame_spec.num_channels());

    return config;
}

} // namespace

TEST_GROUP(sinks) {};

// Don't specify output spec (sink will use default).
TEST(sinks, spec_empty) {
    audio::SampleSpec file_spec;
    file_spec.clear();

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

        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena, "file",
                               file.path(), make_config(file_spec, frame_spec), sink);

        audio::SampleSpec actual_spec = sink->sample_spec();
        CHECK(actual_spec.pcm_subformat() != audio::PcmSubformat_Invalid);
        actual_spec.set_pcm_subformat(audio::PcmSubformat_Raw);

        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!sink->has_state());
        CHECK(!sink->has_latency());
        CHECK(!sink->has_clock());

        write_samples(*sink, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, sink->close());

        read_wav(backend, frame_spec, file.path(), ManySamples);
    }
}

// Specify complete output spec.
TEST(sinks, spec_complete) {
    audio::SampleSpec file_spec;
    file_spec.set_format(audio::Format_Wav);
    file_spec.set_pcm_subformat(audio::PcmSubformat_SInt24);
    file_spec.set_sample_rate(48000);
    file_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Mono));

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

        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena, "file",
                               file.path(), make_config(file_spec, frame_spec), sink);

        audio::SampleSpec actual_spec = sink->sample_spec();
        CHECK(actual_spec.pcm_subformat() != audio::PcmSubformat_Invalid);
        if (actual_spec.pcm_subformat() == audio::PcmSubformat_SInt24_Le) {
            // Sink may request either raw samples or specified output spec.
            actual_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
        }

        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!sink->has_state());
        CHECK(!sink->has_latency());
        CHECK(!sink->has_clock());

        write_samples(*sink, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, sink->close());

        read_wav(backend, frame_spec, file.path(), ManySamples);
    }
}

// Specify only format.
TEST(sinks, spec_only_format) {
    audio::SampleSpec file_spec;
    file_spec.set_format(audio::Format_Wav);

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

        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena, "file",
                               file.path(), make_config(file_spec, frame_spec), sink);

        audio::SampleSpec actual_spec = sink->sample_spec();
        CHECK(actual_spec.pcm_subformat() != audio::PcmSubformat_Invalid);
        actual_spec.set_pcm_subformat(audio::PcmSubformat_Raw);

        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!sink->has_state());
        CHECK(!sink->has_latency());
        CHECK(!sink->has_clock());

        write_samples(*sink, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, sink->close());

        read_wav(backend, frame_spec, file.path(), ManySamples);
    }
}

// Specify only format and sub-format.
TEST(sinks, spec_only_format_and_subformat) {
    audio::SampleSpec file_spec;
    file_spec.set_format(audio::Format_Wav);
    file_spec.set_pcm_subformat(audio::PcmSubformat_SInt24);

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

        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena, "file",
                               file.path(), make_config(file_spec, frame_spec), sink);

        audio::SampleSpec actual_spec = sink->sample_spec();
        CHECK(actual_spec.pcm_subformat() != audio::PcmSubformat_Invalid);
        if (actual_spec.pcm_subformat() == audio::PcmSubformat_SInt24_Le) {
            // Sink may request either raw samples or specified sub-format.
            actual_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
        }

        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!sink->has_state());
        CHECK(!sink->has_latency());
        CHECK(!sink->has_clock());

        write_samples(*sink, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, sink->close());

        read_wav(backend, frame_spec, file.path(), ManySamples);
    }
}

// Specify only sample rate.
TEST(sinks, spec_only_rate) {
    audio::SampleSpec file_spec;
    file_spec.set_sample_rate(48000);

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(48000);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Stereo));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");

        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena, "file",
                               file.path(), make_config(file_spec, frame_spec), sink);

        audio::SampleSpec actual_spec = sink->sample_spec();
        CHECK(actual_spec.pcm_subformat() != audio::PcmSubformat_Invalid);
        actual_spec.set_pcm_subformat(audio::PcmSubformat_Raw);

        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!sink->has_state());
        CHECK(!sink->has_latency());
        CHECK(!sink->has_clock());

        write_samples(*sink, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, sink->close());

        read_wav(backend, frame_spec, file.path(), ManySamples);
    }
}

// Specify only channel set.
TEST(sinks, spec_only_channels) {
    audio::SampleSpec file_spec;
    file_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Mono));

    audio::SampleSpec frame_spec;
    frame_spec.set_format(audio::Format_Pcm);
    frame_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
    frame_spec.set_sample_rate(44100);
    frame_spec.set_channel_set(make_channel_set(audio::ChanMask_Surround_Mono));

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");

        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusOK, backend, frame_factory, arena, "file",
                               file.path(), make_config(file_spec, frame_spec), sink);

        audio::SampleSpec actual_spec = sink->sample_spec();
        CHECK(actual_spec.pcm_subformat() != audio::PcmSubformat_Invalid);
        actual_spec.set_pcm_subformat(audio::PcmSubformat_Raw);

        test::expect_specs_equal(backend.name(), frame_spec, actual_spec);

        CHECK(!sink->has_state());
        CHECK(!sink->has_latency());
        CHECK(!sink->has_clock());

        write_samples(*sink, frame_spec, ManySamples);
        LONGS_EQUAL(status::StatusOK, sink->close());

        read_wav(backend, frame_spec, file.path(), ManySamples);
    }
}

// Directory doesn't exist.
TEST(sinks, bad_path) {
    audio::SampleSpec file_spec;
    file_spec.clear();

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
        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusErrFile, backend, frame_factory, arena,
                               "file", "/bad/file.wav",
                               make_config(file_spec, frame_spec), sink);
    }
}

// File extension not supported by backend.
TEST(sinks, bad_extension) {
    audio::SampleSpec file_spec;
    file_spec.clear();

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

        dbgio::TempFile file("test.bad_ext");

        // Expect StatusNoFormat, which indicates that another backend should be tried
        // (another backend may support this extension).
        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusNoFormat, backend, frame_factory, arena,
                               "file", file.path(), make_config(file_spec, frame_spec),
                               sink);
    }
}

// Format not supported by backend.
TEST(sinks, bad_format) {
    audio::SampleSpec file_spec;
    CHECK(file_spec.set_custom_format("bad_fmt"));

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

        // Expect StatusNoFormat, which indicates that another backend should be tried
        // (another backend may support this format).
        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusNoFormat, backend, frame_factory, arena,
                               "file", file.path(), make_config(file_spec, frame_spec),
                               sink);
    }
}

// Sub-format not allowed by format.
TEST(sinks, bad_subformat) {
    audio::SampleSpec file_spec;
    file_spec.set_format(audio::Format_Wav);
    file_spec.set_pcm_subformat(audio::PcmSubformat_SInt18_3_Be);

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

        // Expect StatusBadConfig, which indicates that there's no need to try other
        // backends (because requested combination of format+subformat is just invalid).
        core::ScopedPtr<ISink> sink;
        test::expect_open_sink(status::StatusBadConfig, backend, frame_factory, arena,
                               "file", file.path(), make_config(file_spec, frame_spec),
                               sink);
    }
}

} // namespace sndio
} // namespace roc
