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

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/config.h"
#include "roc_sndio/pump.h"

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 512, SampleRate = 48000, ChMask = 0x3 };

const audio::SampleSpec sample_spec(SampleRate,
                                    audio::Sample_RawFormat,
                                    audio::ChanLayout_Surround,
                                    audio::ChanOrder_Smpte,
                                    ChMask);

const core::nanoseconds_t frame_duration = FrameSize * core::Second
    / core::nanoseconds_t(sample_spec.sample_rate() * sample_spec.num_channels());

core::HeapArena arena;
core::SlabPool<core::Buffer> buffer_pool(
    "buffer_pool", arena, sizeof(core::Buffer) + FrameSize * sizeof(audio::sample_t));

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

TEST_GROUP(pump) {
    Config source_config;
    Config sink_config;

    void setup() {
        source_config.sample_spec = audio::SampleSpec();

        source_config.frame_length = frame_duration;

        sink_config.sample_spec =
            audio::SampleSpec(SampleRate, audio::Sample_RawFormat,
                              audio::ChanLayout_Surround, audio::ChanOrder_Smpte, ChMask);
        sink_config.frame_length = frame_duration;
    }
};

TEST(pump, write_read) {
    enum { NumSamples = FrameSize * 10 };

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        test::MockSource mock_source;
        mock_source.add(NumSamples);
        core::TempFile file("test.wav");

        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);
            Pump pump(buffer_pool, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusEnd, pump.run());

            CHECK(mock_source.num_returned() >= NumSamples - FrameSize);
        }

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);

        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);
        test::MockSink mock_writer;

        Pump pump(buffer_pool, *backend_source, NULL, mock_writer, frame_duration,
                  sample_spec, Pump::ModePermanent);
        LONGS_EQUAL(status::StatusOK, pump.init_status());
        LONGS_EQUAL(status::StatusEnd, pump.run());

        mock_writer.check(0, mock_source.num_returned());
    }
}

TEST(pump, write_overwrite_read) {
    enum { NumSamples = FrameSize * 10 };

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        test::MockSource mock_source;
        mock_source.add(NumSamples);

        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);
            Pump pump(buffer_pool, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusEnd, pump.run());
        }

        mock_source.add(NumSamples);

        size_t num_returned1 = mock_source.num_returned();
        CHECK(num_returned1 >= NumSamples - FrameSize);

        {
            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);
            Pump pump(buffer_pool, mock_source, NULL, *backend_sink, frame_duration,
                      sample_spec, Pump::ModeOneshot);
            LONGS_EQUAL(status::StatusOK, pump.init_status());
            LONGS_EQUAL(status::StatusEnd, pump.run());
        }

        size_t num_returned2 = mock_source.num_returned() - num_returned1;
        CHECK(num_returned1 >= NumSamples - FrameSize);

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);

        test::MockSink mock_writer;

        Pump pump(buffer_pool, *backend_source, NULL, mock_writer, frame_duration,
                  sample_spec, Pump::ModePermanent);
        LONGS_EQUAL(status::StatusOK, pump.init_status());
        LONGS_EQUAL(status::StatusEnd, pump.run());

        mock_writer.check(num_returned1, num_returned2);
    }
}
} // namespace sndio
} // namespace roc
