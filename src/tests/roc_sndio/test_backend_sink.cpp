/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/pump.h"

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 500, SampleRate = 48000, ChMask = 0x3 };

core::HeapArena arena;

} // namespace

TEST_GROUP(backend_sink) {
    Config sink_config;

    void setup() {
        sink_config.sample_spec =
            audio::SampleSpec(SampleRate, audio::Sample_RawFormat,
                              audio::ChanLayout_Surround, audio::ChanOrder_Smpte, ChMask);

        sink_config.frame_length = FrameSize * core::Second
            / core::nanoseconds_t(sink_config.sample_spec.sample_rate()
                                  * sink_config.sample_spec.num_channels());
    }

    bool supports_wav(IBackend & backend) {
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
};

TEST(backend_sink, open) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!supports_wav(backend)) {
            continue;
        }
        core::TempFile file("test.wav");
        IDevice* backend_device = backend.open_device(
            DeviceType_Sink, DriverType_File, NULL, file.path(), sink_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
        CHECK(backend_sink != NULL);
    }
}

TEST(backend_sink, error) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!supports_wav(backend)) {
            continue;
        }
        IDevice* backend_device = backend.open_device(
            DeviceType_Sink, DriverType_File, NULL, "/bad/file", sink_config, arena);
        CHECK(backend_device == NULL);
    }
}

TEST(backend_sink, has_clock) {
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!supports_wav(backend)) {
            continue;
        }
        IDevice* backend_device = backend.open_device(
            DeviceType_Sink, DriverType_File, NULL, file.path(), sink_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
        CHECK(backend_sink != NULL);
        CHECK(!backend_sink->has_clock());
    }
}

TEST(backend_sink, sample_rate_auto) {
    sink_config.sample_spec.set_sample_rate(0);
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!supports_wav(backend)) {
            continue;
        }
        IDevice* backend_device = backend.open_device(
            DeviceType_Sink, DriverType_File, NULL, file.path(), sink_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
        CHECK(backend_sink != NULL);

        CHECK(backend_sink->sample_spec().sample_rate() != 0);
    }
}

TEST(backend_sink, sample_rate_force) {
    sink_config.sample_spec.set_sample_rate(SampleRate);
    for (size_t n_backend = 0; n_backend < BackendMap::instance().num_backends();
         n_backend++) {
        core::TempFile file("test.wav");
        IBackend& backend = BackendMap::instance().nth_backend(n_backend);
        if (!supports_wav(backend)) {
            continue;
        }
        IDevice* backend_device = backend.open_device(
            DeviceType_Sink, DriverType_File, NULL, file.path(), sink_config, arena);
        CHECK(backend_device != NULL);
        core::ScopedPtr<ISink> backend_sink(backend_device->to_sink(), arena);
        CHECK(backend_sink != NULL);

        CHECK(backend_sink->sample_spec().sample_rate() == SampleRate);
    }
}

} // namespace sndio
} // namespace roc
