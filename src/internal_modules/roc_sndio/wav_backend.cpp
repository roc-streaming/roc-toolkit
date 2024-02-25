/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define DR_WAV_IMPLEMENTATION

#include "roc_sndio/wav_backend.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/wav_sink.h"
#include "roc_sndio/wav_source.h"

namespace roc {
namespace sndio {

WavBackend::WavBackend() {
}

void WavBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list) {
    if (!driver_list.push_back(
            DriverInfo("wav", DriverType_File,
                       DriverFlag_SupportsSink | DriverFlag_SupportsSource, this))) {
        roc_panic("wav backend: can't add driver");
    }
}

IDevice* WavBackend::open_device(DeviceType device_type,
                                 DriverType driver_type,
                                 const char* driver,
                                 const char* path,
                                 const Config& config,
                                 core::IArena& arena) {
    if (driver_type != DriverType_File
        || (driver != NULL && strcmp(driver, "wav") != 0)) {
        return NULL;
    }

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<WavSink> sink(new (arena) WavSink(arena, config), arena);
        if (!sink || !sink->is_valid()) {
            roc_log(LogDebug, "wav backend: can't construct sink: path=%s", path);
            return NULL;
        }

        if (!sink->open(path)) {
            roc_log(LogDebug, "wav backend: open failed: path=%s", path);
            return NULL;
        }

        return sink.release();
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<WavSource> source(new (arena) WavSource(arena, config), arena);
        if (!source || !source->is_valid()) {
            roc_log(LogDebug, "wav backend: can't construct source: path=%s", path);
            return NULL;
        }

        if (!source->open(path)) {
            roc_log(LogDebug, "wav backend: open failed: path=%s", path);
            return NULL;
        }

        return source.release();
    } break;

    default:
        break;
    }

    roc_panic("wav backend: invalid device type");
}

const char* WavBackend::name() const {
    return "wav";
}

} // namespace sndio
} // namespace roc
