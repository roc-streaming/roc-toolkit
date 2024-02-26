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

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/config.h"
#include "roc_sndio/pump.h"
#ifdef ROC_TARGET_SNDFILE
#include "roc_sndio/sndfile_sink.h"
#include "roc_sndio/sndfile_source.h"
#endif // ROC_TARGET_SNDFILE
#ifdef ROC_TARGET_SOX
#include "roc_sndio/sox_sink.h"
#include "roc_sndio/sox_source.h"
#endif // ROC_TARGET_SOX

namespace roc {
namespace sndio {

namespace {

enum { BufSize = 512, SampleRate = 44100, ChMask = 0x3 };

const audio::SampleSpec SampleSpecs(SampleRate,
                                    audio::Sample_RawFormat,
                                    audio::ChanLayout_Surround,
                                    audio::ChanOrder_Smpte,
                                    ChMask);

const core::nanoseconds_t BufDuration = BufSize * core::Second
    / core::nanoseconds_t(SampleSpecs.sample_rate() * SampleSpecs.num_channels());

core::HeapArena arena;
core::BufferFactory<audio::sample_t> buffer_factory(arena, BufSize);

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

        source_config.frame_length = BufDuration;

        sink_config.sample_spec =
            audio::SampleSpec(SampleRate, audio::Sample_RawFormat,
                              audio::ChanLayout_Surround, audio::ChanOrder_Smpte, ChMask);
        sink_config.frame_length = BufDuration;
    }
};

TEST(pump, write_read) {
    enum { NumSamples = BufSize * 10 };

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
            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, BufDuration,
                      SampleSpecs, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());

            CHECK(mock_source.num_returned() >= NumSamples - BufSize);
        }

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);

        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);
        test::MockSink mock_writer;

        Pump pump(buffer_factory, *backend_source, NULL, mock_writer, BufDuration,
                  SampleSpecs, Pump::ModePermanent);
        CHECK(pump.is_valid());
        CHECK(pump.run());

        mock_writer.check(0, mock_source.num_returned());
    }
}

TEST(pump, write_overwrite_read) {
    enum { NumSamples = BufSize * 10 };

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
            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, BufDuration,
                      SampleSpecs, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        mock_source.add(NumSamples);

        size_t num_returned1 = mock_source.num_returned();
        CHECK(num_returned1 >= NumSamples - BufSize);

        {
            IDevice* backend_device = backend.open_device(
                DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);
            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, BufDuration,
                      SampleSpecs, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        size_t num_returned2 = mock_source.num_returned() - num_returned1;
        CHECK(num_returned1 >= NumSamples - BufSize);

        IDevice* backend_device = backend.open_device(
            DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);

        test::MockSink mock_writer;

        Pump pump(buffer_factory, *backend_source, NULL, mock_writer, BufDuration,
                  SampleSpecs, Pump::ModePermanent);
        CHECK(pump.is_valid());
        CHECK(pump.run());

        mock_writer.check(num_returned1, num_returned2);
    }
}
} // namespace sndio
} // namespace roc
