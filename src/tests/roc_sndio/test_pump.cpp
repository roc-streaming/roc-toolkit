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

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_dbgio/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/config.h"
#include "roc_sndio/pump.h"

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 512, SampleRate = 48000 };

const audio::SampleSpec sample_spec(SampleRate,
                                    audio::Sample_RawFormat,
                                    audio::ChanLayout_Surround,
                                    audio::ChanOrder_Smpte,
                                    audio::ChanMask_Surround_Stereo);

const core::nanoseconds_t frame_duration = FrameSize * core::Second
    / core::nanoseconds_t(sample_spec.sample_rate() * sample_spec.num_channels());

core::HeapArena arena;

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer> frame_buffer_pool("frame_buffer_pool",
                                               arena,
                                               sizeof(core::Buffer)
                                                   + FrameSize * sizeof(audio::sample_t));

audio::FrameFactory frame_factory(frame_pool, frame_buffer_pool);

} // namespace

TEST_GROUP(pump) {
    Config source_config;
    Config sink_config;

    void setup() {
        source_config.sample_spec = audio::SampleSpec();
        source_config.frame_length = frame_duration;

        sink_config.sample_spec = sample_spec;
        sink_config.frame_length = frame_duration;
    }
};

TEST(pump, write_read) {
    enum { NumSamples = FrameSize * 10 };

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");

        test::MockSource mock_source(frame_factory, sink_config.sample_spec);
        mock_source.add(NumSamples);

        {
            // open sink
            core::ScopedPtr<ISink> backend_sink;
            test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                                   DriverType_File, "wav", file.path(), sink_config,
                                   backend_sink);

            // copy from mock source to sink
            Pump pump(frame_pool, frame_buffer_pool, mock_source, NULL, *backend_sink,
                      sink_config, Pump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusOK, pump.run());

            CHECK(mock_source.num_returned() >= NumSamples - FrameSize);
        }

        // open source
        core::ScopedPtr<ISource> backend_source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena,
                                 DriverType_File, "wav", file.path(), source_config,
                                 backend_source);

        // copy from source to mock sink
        test::MockSink mock_sink;
        Pump pump(frame_pool, frame_buffer_pool, *backend_source, NULL, mock_sink,
                  sink_config, Pump::ModePermanent);
        LONGS_EQUAL(status::StatusOK, pump.init_status());
        LONGS_EQUAL(status::StatusOK, pump.run());

        // check samples
        mock_sink.check(0, mock_source.num_returned());
    }
}

TEST(pump, write_overwrite_read) {
    enum { NumSamples = FrameSize * 10 };

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");

        test::MockSource mock_source(frame_factory, sink_config.sample_spec);
        mock_source.add(NumSamples);

        {
            // open sink
            core::ScopedPtr<ISink> backend_sink;
            test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                                   DriverType_File, "wav", file.path(), sink_config,
                                   backend_sink);

            // copy from mock source to sink
            Pump pump(frame_pool, frame_buffer_pool, mock_source, NULL, *backend_sink,
                      sink_config, Pump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusOK, pump.run());
        }

        // add more samples to mock source
        mock_source.add(NumSamples);

        size_t num_returned1 = mock_source.num_returned();
        CHECK(num_returned1 >= NumSamples - FrameSize);

        {
            // open sink
            core::ScopedPtr<ISink> backend_sink;
            test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                                   DriverType_File, "wav", file.path(), sink_config,
                                   backend_sink);

            // copy next samples from mock source to sink, overwriting file
            Pump pump(frame_pool, frame_buffer_pool, mock_source, NULL, *backend_sink,
                      sink_config, Pump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusOK, pump.run());
        }

        size_t num_returned2 = mock_source.num_returned() - num_returned1;
        CHECK(num_returned1 >= NumSamples - FrameSize);

        // open source
        core::ScopedPtr<ISource> backend_source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena,
                                 DriverType_File, "wav", file.path(), source_config,
                                 backend_source);

        // copy from source to mock sink
        test::MockSink mock_sink;
        Pump pump(frame_pool, frame_buffer_pool, *backend_source, NULL, mock_sink,
                  sink_config, Pump::ModePermanent);
        LONGS_EQUAL(status::StatusOK, pump.init_status());
        LONGS_EQUAL(status::StatusOK, pump.run());

        // check samples
        mock_sink.check(num_returned1, num_returned2);
    }
}
} // namespace sndio
} // namespace roc
