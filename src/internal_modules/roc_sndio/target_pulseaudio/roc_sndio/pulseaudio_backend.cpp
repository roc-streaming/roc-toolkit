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

bool PulseaudioBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& result) {
    if (!result.push_back(DriverInfo("pulse",
                                     Driver_Device | Driver_DefaultDevice
                                         | Driver_SupportsSink | Driver_SupportsSource,
                                     this))) {
        return false;
    }
    return true;
}

bool PulseaudioBackend::discover_formats(core::Array<FormatInfo, MaxFormats>& result) {
    // no formats except pcm
    return true;
}

bool PulseaudioBackend::discover_subformat_groups(core::StringList& result) {
    // no sub-formats except pcm
    return true;
}

bool PulseaudioBackend::discover_subformats(const char* group, core::StringList& result) {
    // no sub-formats except pcm
    return true;
}

status::StatusCode PulseaudioBackend::open_device(DeviceType device_type,
                                                  const char* driver,
                                                  const char* path,
                                                  const IoConfig& io_config,
                                                  audio::FrameFactory& frame_factory,
                                                  core::IArena& arena,
                                                  IDevice** result) {
    roc_panic_if(!driver);
    roc_panic_if(!path);

    if (strcmp(driver, "pulse") != 0) {
        // Not pulse://, go to next backend.
        return status::StatusNoDriver;
    }

    core::ScopedPtr<PulseaudioDevice> device(
        new (arena) PulseaudioDevice(frame_factory, arena, io_config, device_type, path));

    if (!device) {
        roc_log(LogDebug, "pulseaudio backend: can't allocate device: path=%s", path);
        return status::StatusNoMem;
    }

    if (device->init_status() != status::StatusOK) {
        roc_log(LogDebug, "pulseaudio backend: can't open device: path=%s status=%s",
                path, status::code_to_str(device->init_status()));
        return device->init_status();
    }

    *result = device.hijack();
    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
