/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/wav_backend.h"
#include "roc_sndio/wav_sink.h"
#include "roc_sndio/wav_source.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

bool has_suffix(const char* str, const char* suffix) {
    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);
    if (len_suffix > len_str) {
        return false;
    }
    return strncmp(str + len_str - len_suffix, suffix, len_suffix) == 0;
}

} // namespace

WavBackend::WavBackend() {
}

const char* WavBackend::name() const {
    return "wav";
}

void WavBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list) {
    if (!driver_list.push_back(
            DriverInfo("wav", DriverType_File,
                       DriverFlag_SupportsSink | DriverFlag_SupportsSource, this))) {
        roc_panic("wav backend: can't add driver");
    }
}

status::StatusCode WavBackend::open_device(DeviceType device_type,
                                           DriverType driver_type,
                                           const char* driver,
                                           const char* path,
                                           const Config& config,
                                           audio::FrameFactory& frame_factory,
                                           core::IArena& arena,
                                           IDevice** result) {
    if (driver_type != DriverType_File) {
        return status::StatusNoDriver;
    }

    if (driver) {
        if (strcmp(driver, "wav") != 0) {
            return status::StatusNoDriver;
        }
    } else {
        if (!has_suffix(path, ".wav")) {
            return status::StatusNoDriver;
        }
    }

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<WavSink> sink(new (arena) WavSink(frame_factory, arena, config),
                                      arena);

        if (!sink) {
            roc_log(LogDebug, "wav backend: can't allocate sink: path=%s", path);
            return status::StatusNoMem;
        }

        if (sink->init_status() != status::StatusOK) {
            roc_log(LogDebug, "wav backend: can't initialize sink: path=%s status=%s",
                    path, status::code_to_str(sink->init_status()));
            return sink->init_status();
        }

        const status::StatusCode code = sink->open(path);
        if (code != status::StatusOK) {
            roc_log(LogDebug, "wav backend: can't open sink: path=%s status=%s", path,
                    status::code_to_str(code));
            return code;
        }

        *result = sink.release();
        return status::StatusOK;
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<WavSource> source(
            new (arena) WavSource(frame_factory, arena, config), arena);

        if (!source) {
            roc_log(LogDebug, "wav backend: can't allocate source: path=%s", path);
            return status::StatusNoMem;
        }

        if (source->init_status() != status::StatusOK) {
            roc_log(LogDebug, "wav backend: can't initialize source: path=%s status=%s",
                    path, status::code_to_str(source->init_status()));
            return source->init_status();
        }

        const status::StatusCode code = source->open(path);
        if (code != status::StatusOK) {
            roc_log(LogDebug, "wav backend: can't open source: path=%s status=%s", path,
                    status::code_to_str(code));
            return code;
        }

        *result = source.release();
        return status::StatusOK;
    } break;

    default:
        break;
    }

    roc_panic("wav backend: invalid device type");
}

} // namespace sndio
} // namespace roc
