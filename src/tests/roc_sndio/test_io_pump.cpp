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
#include "roc_sndio/io_config.h"
#include "roc_sndio/io_pump.h"

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 512, ManySamples = FrameSize * 10 };

const audio::SampleSpec frame_spec(48000,
                                   audio::PcmSubformat_Raw,
                                   audio::ChanLayout_Surround,
                                   audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Stereo);

const core::nanoseconds_t frame_duration = FrameSize * core::Second
    / core::nanoseconds_t(frame_spec.sample_rate() * frame_spec.num_channels());

core::HeapArena arena;

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer> frame_buffer_pool("frame_buffer_pool",
                                               arena,
                                               sizeof(core::Buffer)
                                                   + FrameSize * sizeof(audio::sample_t));

audio::FrameFactory frame_factory(frame_pool, frame_buffer_pool);

} // namespace

TEST_GROUP(io_pump) {
    IoConfig source_config;
    IoConfig sink_config;
    IoConfig pump_config;

    void setup() {
        audio::SampleSpec file_spec;
        file_spec.set_format(audio::Format_Wav);
        file_spec.set_pcm_subformat(audio::PcmSubformat_Raw);
        file_spec.set_sample_rate(frame_spec.sample_rate());
        file_spec.set_channel_set(frame_spec.channel_set());

        source_config.sample_spec = audio::SampleSpec();
        source_config.frame_length = frame_duration;

        sink_config.sample_spec = file_spec;
        sink_config.frame_length = frame_duration;

        pump_config.sample_spec = frame_spec;
        pump_config.frame_length = frame_duration;
    }
};

TEST(io_pump, write_read) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");

        test::MockSource mock_source(pump_config.sample_spec, frame_factory, arena);
        mock_source.add(ManySamples);

        {
            // open file sink
            core::ScopedPtr<ISink> file_sink;
            test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                                   "file", file.path(), sink_config, file_sink);

            // copy from mock source to file sink
            IoPump pump(frame_pool, frame_buffer_pool, mock_source, NULL, *file_sink,
                        pump_config, IoPump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusOK, pump.run());

            CHECK(mock_source.num_returned() >= ManySamples - FrameSize);
        }

        // open file source
        core::ScopedPtr<ISource> file_source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                                 file.path(), source_config, file_source);

        // copy from file source to mock sink
        test::MockSink mock_sink(arena);
        IoPump pump(frame_pool, frame_buffer_pool, *file_source, NULL, mock_sink,
                    pump_config, IoPump::ModePermanent);
        LONGS_EQUAL(status::StatusOK, pump.init_status());
        LONGS_EQUAL(status::StatusOK, pump.run());

        // check samples
        mock_sink.check(0, mock_source.num_returned());
    }
}

TEST(io_pump, write_overwrite_read) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!test::backend_supports_format(backend, arena, "wav")) {
            continue;
        }

        dbgio::TempFile file("test.wav");

        test::MockSource mock_source(pump_config.sample_spec, frame_factory, arena);
        mock_source.add(ManySamples);

        {
            // open file sink
            core::ScopedPtr<ISink> file_sink;
            test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                                   "file", file.path(), sink_config, file_sink);

            // copy from mock source to file sink
            IoPump pump(frame_pool, frame_buffer_pool, mock_source, NULL, *file_sink,
                        pump_config, IoPump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusOK, pump.run());
        }

        // add more samples to mock source
        mock_source.add(ManySamples);

        size_t num_returned1 = mock_source.num_returned();
        CHECK(num_returned1 >= ManySamples - FrameSize);

        {
            // open file sink
            core::ScopedPtr<ISink> file_sink;
            test::expect_open_sink(status::StatusOK, backend, frame_factory, arena,
                                   "file", file.path(), sink_config, file_sink);

            // copy next samples from mock source to file sink, overwriting file
            IoPump pump(frame_pool, frame_buffer_pool, mock_source, NULL, *file_sink,
                        pump_config, IoPump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusOK, pump.run());
        }

        size_t num_returned2 = mock_source.num_returned() - num_returned1;
        CHECK(num_returned1 >= ManySamples - FrameSize);

        // open file source
        core::ScopedPtr<ISource> file_source;
        test::expect_open_source(status::StatusOK, backend, frame_factory, arena, "file",
                                 file.path(), source_config, file_source);

        // copy from file source to mock sink
        test::MockSink mock_sink(arena);
        IoPump pump(frame_pool, frame_buffer_pool, *file_source, NULL, mock_sink,
                    pump_config, IoPump::ModePermanent);
        LONGS_EQUAL(status::StatusOK, pump.init_status());
        LONGS_EQUAL(status::StatusOK, pump.run());

        // check samples
        mock_sink.check(num_returned1, num_returned2);
    }
}

} // namespace sndio
} // namespace roc
