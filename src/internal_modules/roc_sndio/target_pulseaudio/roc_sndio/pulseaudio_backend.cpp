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
#include "roc_sndio/pulseaudio_device.h"

namespace roc {
namespace sndio {

PulseaudioBackend::PulseaudioBackend() {
}

void PulseaudioBackend::discover_drivers(
    core::Array<DriverInfo, MaxDrivers>& driver_list) {
    if (!driver_list.push_back(DriverInfo("pulse", DriverType_Device,
                                          DriverFlag_IsDefault | DriverFlag_SupportsSink
                                              | DriverFlag_SupportsSource,
                                          this))) {
        roc_panic("pulseaudio backend: can't add driver");
    }
}

IDevice* PulseaudioBackend::open_device(DeviceType device_type,
                                        DriverType driver_type,
                                        const char* driver,
                                        const char* path,
                                        const Config& config,
                                        core::IArena& arena) {
    if (driver_type != DriverType_Device) {
        return NULL;
    }

    if (driver && strcmp(driver, "pulse") != 0) {
        return NULL;
    }

    core::ScopedPtr<PulseaudioDevice> device(
        new (arena) PulseaudioDevice(config, device_type), arena);

    if (!device) {
        roc_log(LogDebug, "pulseaudio backend: can't construct device: path=%s", path);
        return NULL;
    }

    if (!device->open(path)) {
        roc_log(LogDebug, "pulseaudio backend: can't open device: path=%s", path);
        return NULL;
    }

    return device.release();
}

const char* PulseaudioBackend::name() const {
    return "pulseaudio";
}

} // namespace sndio
} // namespace roc
