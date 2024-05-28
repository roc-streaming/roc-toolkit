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
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

PulseaudioBackend::PulseaudioBackend() {
}

const char* PulseaudioBackend::name() const {
    return "pulseaudio";
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

status::StatusCode PulseaudioBackend::open_device(DeviceType device_type,
                                                  DriverType driver_type,
                                                  const char* driver,
                                                  const char* path,
                                                  const Config& config,
                                                  audio::FrameFactory& frame_factory,
                                                  core::IArena& arena,
                                                  IDevice** result) {
    if (driver_type != DriverType_Device) {
        return status::StatusNoDriver;
    }

    if (driver && strcmp(driver, "pulse") != 0) {
        return status::StatusNoDriver;
    }

    core::ScopedPtr<PulseaudioDevice> device(
        new (arena) PulseaudioDevice(frame_factory, arena, config, device_type), arena);

    if (!device) {
        roc_log(LogDebug, "pulseaudio backend: can't allocate device: path=%s", path);
        return status::StatusNoMem;
    }

    if (device->init_status() != status::StatusOK) {
        roc_log(LogDebug,
                "pulseaudio backend: can't initialize device: path=%s status=%s", path,
                status::code_to_str(device->init_status()));
        return device->init_status();
    }

    const status::StatusCode code = device->open(path);
    if (code != status::StatusOK) {
        roc_log(LogDebug, "pulseaudio backend: can't open device: path=%s status=%s",
                path, status::code_to_str(code));
        return code;
    }

    *result = device.release();
    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
