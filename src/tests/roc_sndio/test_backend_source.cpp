/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_source.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/pump.h"

namespace roc {
namespace sndio {

namespace {

enum {
    MaxBufSize = 8192,
    FrameSize = 500,
    SampleRate = 48000,
    ChMask = 0x3,
    NumChans = 2
};

const audio::SampleSpec sample_spec(SampleRate,
                                    audio::Sample_RawFormat,
                                    audio::ChanLayout_Surround,
                                    audio::ChanOrder_Smpte,
                                    ChMask);

const core::nanoseconds_t frame_duration = FrameSize * core::Second
    / core::nanoseconds_t(sample_spec.sample_rate() * sample_spec.num_channels());

core::HeapArena arena;
core::BufferFactory<audio::sample_t> buffer_factory(arena, MaxBufSize);

bool supports_wav(IBackend& backend) {
    bool supports = false;
    core::Array<DriverInfo, MaxDrivers> driver_list(arena);
    backend.discover_drivers(driver_list);
    for (size_t n = 0; n < driver_list.size(); n++) {
        if (strcmp(driver_list[n].name, "wav") == 0) {
            supports = true;
            break;
        }
    }

    return supports;
}

} // namespace

TEST_GROUP(backend_source) {
    Config sink_config;
    Config source_config;

    void setup() {
        sink_config.sample_spec =
            audio::SampleSpec(SampleRate, audio::Sample_RawFormat,
                              audio::ChanLayout_Surround, audio::ChanOrder_Smpte, ChMask);
        sink_config.frame_length = frame_duration;

        source_config.sample_spec = audio::SampleSpec();
        source_config.frame_length = frame_duration;
    }
};

TEST(backend_source, open) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }
        {
            test::MockSource mock_source;
            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, NULL, file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }
        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, NULL, file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);
    }
}

TEST(backend_source, error) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!supports_wav(backend)) {
            continue;
        }
        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, NULL, "/bad/file", source_config, arena);
        CHECK(backend_device == NULL);
    }
}

TEST(backend_source, has_clock) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(MaxBufSize * 10);

            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, NULL, file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, NULL, file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);
        CHECK(!backend_source->has_clock());
    }
}

TEST(backend_source, sample_rate_auto) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(MaxBufSize * 10);

            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }
        source_config.sample_spec.set_sample_rate(0);
        source_config.frame_length = frame_duration;

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);

        CHECK(backend_source->sample_spec().sample_rate() == SampleRate);
    }
}

TEST(backend_source, sample_rate_mismatch) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(MaxBufSize * 10);

            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        source_config.sample_spec.set_sample_rate(SampleRate * 2);

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);

        CHECK(backend_device == NULL);
    }
}

TEST(backend_source, pause_resume) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(FrameSize * NumChans * 2);

            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);

        audio::sample_t frame_data1[FrameSize * NumChans] = {};
        audio::Frame frame1(frame_data1, FrameSize * NumChans);

        // TODO(gh-706): check state

        CHECK(backend_source->read(frame1));

        audio::sample_t frame_data2[FrameSize * NumChans] = {};
        audio::Frame frame2(frame_data2, FrameSize * NumChans);

        backend_source->pause();
        if (strcmp(backend.name(), "sox") == 0) {
            // TODO(gh-706): check state

            CHECK(!backend_source->read(frame2));

            CHECK(backend_source->resume());
            // TODO(gh-706): check state

            CHECK(backend_source->read(frame2));
        } else {
            // TODO(gh-706): check state

            CHECK(backend_source->read(frame2));

            CHECK(backend_source->resume());
            // TODO(gh-706): check state

            CHECK(!backend_source->read(frame2));
        }

        if (memcmp(frame_data1, frame_data2, sizeof(frame_data1)) == 0) {
            FAIL("frames should not be equal");
        }
    }
}

TEST(backend_source, pause_restart) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(FrameSize * NumChans * 2);

            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);

        audio::sample_t frame_data1[FrameSize * NumChans] = {};
        audio::Frame frame1(frame_data1, FrameSize * NumChans);

        // TODO(gh-706): check state

        CHECK(backend_source->read(frame1));

        backend_source->pause();

        audio::sample_t frame_data2[FrameSize * NumChans] = {};
        audio::Frame frame2(frame_data2, FrameSize * NumChans);

        if (strcmp(backend.name(), "sox") == 0) {
            // TODO(gh-706): check state

            CHECK(!backend_source->read(frame2));

            CHECK(backend_source->restart());
            // TODO(gh-706): check state

            CHECK(backend_source->read(frame2));
        } else {
            // TODO(gh-706): check state

            CHECK(backend_source->read(frame2));

            CHECK(backend_source->restart());
            // TODO(gh-706): check state

            CHECK(backend_source->read(frame2));
        }

        if (memcmp(frame_data1, frame_data2, sizeof(frame_data1)) != 0) {
            FAIL("frames should be equal");
        }
    }
}

TEST(backend_source, eof_restart) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(FrameSize * NumChans * 2);

            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);

        audio::sample_t frame_data[FrameSize * NumChans] = {};
        audio::Frame frame(frame_data, FrameSize * NumChans);

        for (int i = 0; i < 3; i++) {
            CHECK(backend_source->read(frame));
            CHECK(backend_source->read(frame));
            CHECK(!backend_source->read(frame));

            CHECK(backend_source->restart());
        }
    }
}

} // namespace sndio

} // namespace roc
