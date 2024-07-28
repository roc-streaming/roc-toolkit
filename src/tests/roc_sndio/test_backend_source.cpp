/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_source.h"
#include "test_helpers/utils.h"

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/slab_pool.h"
#include "roc_dbgio/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/pump.h"

namespace roc {
namespace sndio {

namespace {

enum { MaxBufSize = 8192, FrameSize = 500, SampleRate = 48000 };

const audio::SampleSpec sample_spec(SampleRate,
                                    audio::Sample_RawFormat,
                                    audio::ChanLayout_Surround,
                                    audio::ChanOrder_Smpte,
                                    audio::ChanMask_Surround_Stereo);

const core::nanoseconds_t frame_duration = FrameSize * core::Second
    / core::nanoseconds_t(sample_spec.sample_rate() * sample_spec.num_channels());

core::HeapArena arena;

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer>
    frame_buffer_pool("frame_buffer_pool",
                      arena,
                      sizeof(core::Buffer) + MaxBufSize * sizeof(audio::sample_t));

audio::FrameFactory frame_factory(frame_pool, frame_buffer_pool);

void write_wav(IBackend& backend,
               const Config& config,
               const char* path,
               size_t num_samples) {
    test::MockSource mock_source(frame_factory, config.sample_spec);
    mock_source.add(num_samples * sample_spec.num_channels());

    IDevice* backend_device = NULL;
    LONGS_EQUAL(status::StatusOK,
                backend.open_device(DeviceType_Sink, DriverType_File, NULL, path, config,
                                    frame_factory, arena, &backend_device));
    CHECK(backend_device != NULL);
    core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
    CHECK(backend_sink != NULL);

    Pump pump(frame_pool, frame_buffer_pool, mock_source, NULL, *backend_sink, config,
              Pump::ModeOneshot);
    LONGS_EQUAL(status::StatusOK, pump.init_status());
    LONGS_EQUAL(status::StatusOK, pump.run());
}

void expect_read(status::StatusCode expected_code,
                 ISource& source,
                 audio::Frame& frame,
                 packet::stream_timestamp_t requested_samples) {
    const status::StatusCode code =
        source.read(frame, requested_samples, audio::ModeHard);

    LONGS_EQUAL(expected_code, code);
}

} // namespace

TEST_GROUP(backend_source) {
    Config sink_config;
    Config source_config;

    void setup() {
        sink_config.sample_spec = sample_spec;
        sink_config.frame_length = frame_duration;

        source_config.sample_spec = audio::SampleSpec();
        source_config.frame_length = frame_duration;
    }
};

TEST(backend_source, open) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, sink_config, file.path(), MaxBufSize * 10);

        core::ScopedPtr<ISource> backend_source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena,
                                 DriverType_File, NULL, file.path(), source_config,
                                 backend_source);

        test::expect_specs_equal(backend.name(), sink_config.sample_spec,
                                 backend_source->sample_spec());

        CHECK(!backend_source->has_state());
        CHECK(!backend_source->has_latency());
        CHECK(!backend_source->has_clock());
        LONGS_EQUAL(status::StatusOK, backend_source->close());
    }
}

// Open fails because file doesn't exist.
TEST(backend_source, open_bad_file) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        core::ScopedPtr<ISource> backend_source;
        test::expect_open_source(status::StatusErrFile, backend, frame_factory, arena,
                                 DriverType_File, NULL, "/bad/file.wav", source_config,
                                 backend_source);
    }
}

// Open fails because of invalid sndio::Config.
TEST(backend_source, open_bad_config) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, sink_config, file.path(), MaxBufSize * 10);

        Config bad_config = source_config;
        bad_config.sample_spec.set_sample_rate(SampleRate);

        core::ScopedPtr<ISource> backend_source;
        test::expect_open_source(status::StatusBadConfig, backend, frame_factory, arena,
                                 DriverType_File, NULL, file.path(), bad_config,
                                 backend_source);
    }
}

// Rewind and read same frame again.
TEST(backend_source, rewind) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, sink_config, file.path(), MaxBufSize * 10);

        core::ScopedPtr<ISource> backend_source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena,
                                 DriverType_File, "wav", file.path(), source_config,
                                 backend_source);

        audio::FramePtr frame1 = frame_factory.allocate_frame_no_buffer();
        CHECK(frame1);
        expect_read(status::StatusOK, *backend_source, *frame1, FrameSize);

        // rewind
        LONGS_EQUAL(status::StatusOK, backend_source->rewind());

        audio::FramePtr frame2 = frame_factory.allocate_frame_no_buffer();
        CHECK(frame2);
        expect_read(status::StatusOK, *backend_source, *frame2, FrameSize);

        LONGS_EQUAL(FrameSize * sample_spec.num_channels(), frame1->num_raw_samples());
        LONGS_EQUAL(FrameSize * sample_spec.num_channels(), frame2->num_raw_samples());

        if (memcmp(frame1->raw_samples(), frame2->raw_samples(),
                   frame1->num_raw_samples() * sizeof(audio::sample_t))
            != 0) {
            FAIL("frames should be equal");
        }
        LONGS_EQUAL(status::StatusOK, backend_source->close());
    }
}

// Read until EOF, rewind, repeat.
TEST(backend_source, rewind_after_eof) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");
        write_wav(backend, sink_config, file.path(), FrameSize * 2);

        core::ScopedPtr<ISource> backend_source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena,
                                 DriverType_File, "wav", file.path(), source_config,
                                 backend_source);

        audio::FramePtr frame = frame_factory.allocate_frame_no_buffer();
        CHECK(frame);

        for (int i = 0; i < 10; i++) {
            expect_read(status::StatusOK, *backend_source, *frame, FrameSize);
            expect_read(status::StatusOK, *backend_source, *frame, FrameSize);
            expect_read(status::StatusFinish, *backend_source, *frame, FrameSize);

            // rewind
            LONGS_EQUAL(status::StatusOK, backend_source->rewind());
        }
        LONGS_EQUAL(status::StatusOK, backend_source->close());
    }
}

} // namespace sndio

} // namespace roc
