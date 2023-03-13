/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pulseaudio_backend.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/pulseaudio_sink.h"
#include "roc_sndio/pulseaudio_source.h"

namespace roc {
namespace sndio {

PulseaudioBackend::PulseaudioBackend() {
    roc_log(LogDebug, "pulseaudio backend: initializing");
}

void PulseaudioBackend::discover_drivers(
    core::Array<DriverInfo, MaxDrivers>& driver_list) {
    if (!driver_list.grow(driver_list.size() + 1)) {
        roc_panic("pulseaudio backend: can't grow drivers array");
    }

    driver_list.push_back(DriverInfo("pulse", DriverType_Device,
                                     DriverFlag_IsDefault | DriverFlag_SupportsSink
                                         | DriverFlag_SupportsSource,
                                     this));
}

IDevice* PulseaudioBackend::open_device(DeviceType device_type,
                                        DriverType driver_type,
                                        const char* driver,
                                        const char* path,
                                        const Config& config,
                                        core::IAllocator& allocator) {
    if (driver_type != DriverType_Device) {
        return NULL;
    }

    if (driver && strcmp(driver, "pulse") != 0) {
        return NULL;
    }

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<PulseaudioSink> sink(new (allocator) PulseaudioSink(config),
                                             allocator);
        if (!sink) {
            roc_log(LogDebug, "pulseaudio backend: can't construct sink: path=%s", path);
            return NULL;
        }

        if (!sink->open(path)) {
            roc_log(LogDebug, "pulseaudio backend: can't open sink: path=%s", path);
            return NULL;
        }

        return sink.release();
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<PulseaudioSource> source(new (allocator) PulseaudioSource(config),
                                                 allocator);
        if (!source) {
            roc_log(LogDebug, "pulseaudio backend: can't construct source: path=%s",
                    path);
            return NULL;
        }

        if (!source->open(path)) {
            roc_log(LogDebug, "pulseaudio backend: can't open source: path=%s", path);
            return NULL;
        }

        return source.release();
    } break;

    default:
        break;
    }

    roc_panic("pulseaudio backend: invalid device type");
}

} // namespace sndio
} // namespace roc
