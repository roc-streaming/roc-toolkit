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
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/backend_map.h"
#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/pump.h"
#include "roc_sndio/sndfile_sink.h"
#include "roc_sndio/sndfile_source.h"
#include "roc_sndio/sox_sink.h"
#include "roc_sndio/sox_source.h"

namespace roc {
namespace sndio {

namespace {

enum {
    MaxBufSize = 8192,
    FrameSize = 500,
    SampleRate = 44100,
    ChMask = 0x3,
    NumChans = 2
};

const audio::SampleSpec
    SampleSpecs(SampleRate, audio::Sample_RawFormat, audio::ChanLayout_Surround, audio::ChanOrder_Smpte, ChMask);

const core::nanoseconds_t FrameDuration = FrameSize * core::Second
    / core::nanoseconds_t(SampleSpecs.sample_rate() * SampleSpecs.num_channels());

core::HeapArena arena;
core::BufferFactory<audio::sample_t> buffer_factory(arena, MaxBufSize);

bool supports_wav(IBackend &backend){
    bool supports = false;
    core::Array<DriverInfo, MaxDrivers> driver_list;
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
        sink_config.sample_spec = audio::SampleSpec(
            SampleRate, audio::Sample_RawFormat, audio::ChanLayout_Surround, audio::ChanOrder_Smpte, ChMask);
        sink_config.frame_length = FrameDuration;

        source_config.sample_spec = audio::SampleSpec();
        source_config.frame_length = FrameDuration;
    }
};

TEST(backend_source, noop) {
    SndfileSource sndfile_source(arena, source_config);
    SoxSource sox_source(arena, source_config);
}

TEST(backend_source, error) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends(); n_backend++) {
        IBackend &backend = BackendMap::instance().nth_backend(n_backend);
        IDevice *backend_device = backend.open_device(DeviceType_Source, DriverType_File, NULL, "/bad/file", sink_config, arena);
        CHECK(backend_device == NULL); 
    }
}

TEST(backend_source, has_clock) {
    core::TempFile file("test.wav");

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends(); n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        
        bool supports_wav = false;
        core::Array<DriverInfo, MaxDrivers> driver_list;
        backend.discover_drivers(driver_list);
        for (size_t n = 0; n < driver_list.size(); n++) {
            if (strcmp(driver_list[n].name, "wav") == 0) {
                supports_wav = true;
                break;
            }
        }
        if(!supports_wav){
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(MaxBufSize * 10);

            IDevice* backend_device = backend.open_device(DeviceType_Sink, DriverType_File, NULL, file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, FrameDuration, SampleSpecs, 
                  Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }

        IDevice* backend_device = backend.open_device(DeviceType_Source, DriverType_File, NULL, file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);
        CHECK(!backend_source->has_clock()); 
    } 
}

TEST(backend_source, sample_rate_auto) {
    core::TempFile file("test.wav");

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends(); n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }
        
        {
            test::MockSource mock_source;
            mock_source.add(MaxBufSize * 10);

            IDevice* backend_device = backend.open_device(DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, FrameDuration, SampleSpecs, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }
        source_config.sample_spec.set_sample_rate(0);
        source_config.frame_length = FrameDuration;

        IDevice *backend_device = backend.open_device(DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source != NULL);

        CHECK(backend_source->sample_spec().sample_rate() == SampleRate);
    }
}

TEST(backend_source, sample_rate_mismatch) {
    core::TempFile file("test.wav");

    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends(); n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);

        if (!supports_wav(backend)) {
            continue;
        }

        {
            test::MockSource mock_source;
            mock_source.add(MaxBufSize * 10);

            IDevice* backend_device = backend.open_device(DeviceType_Sink, DriverType_File, "wav", file.path(), sink_config, arena);
            CHECK(backend_device != NULL);
            core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
            CHECK(backend_sink != NULL);

            Pump pump(buffer_factory, mock_source, NULL, *backend_sink, FrameDuration, SampleSpecs, Pump::ModeOneshot);
            CHECK(pump.is_valid());
            CHECK(pump.run());
        }
        
        source_config.sample_spec.set_sample_rate(SampleRate * 2);

        IDevice * backend_device = backend.open_device(DeviceType_Source, DriverType_File, "wav", file.path(), source_config, arena);
        printf("backend: %s\n", backend.name());
        fflush(stdout);
        if(strcmp(backend.name(), "wav") != 0){
            
        
        CHECK(backend_device == NULL);
        core::ScopedPtr<ISource> backend_source(backend_device->to_source(), arena);
        CHECK(backend_source == NULL);
        }else
        {
            
        }
    }
}

}

} 
