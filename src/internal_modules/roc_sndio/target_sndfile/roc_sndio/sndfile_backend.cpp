/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/scoped_lock.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/sndfile_backend.h"
#include "roc_sndio/sndfile_sink.h"
#include "roc_sndio/sndfile_source.h"

namespace roc {
namespace sndio {

SndfileBackend::SndfileBackend()
    : first_created_(false) {
    roc_log(LogDebug, "sndfile backend: initializing");
}

void SndfileBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list) {
    int total_number_of_drivers;

    if (int errnum = sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT,
                                &total_number_of_drivers, sizeof(int))) {
        roc_panic("sndfile backend: %s", sf_error_number(errnum));
    }

    SF_FORMAT_INFO format_info;

    int wav_count = 0;
    int mat_count = 0;

    for (int n = 0; n < total_number_of_drivers; n++) {
        format_info.format = n;
        if (int errnum = sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info,
                                    sizeof(format_info))) {
            roc_panic("sndfile backend: %s", sf_error_number(errnum));
        }

        const char* driver = format_info.extension;

        if (strcmp(driver, "wav") == 0) {
            if (wav_count == 1) {
                driver = "nist";
            } else if (wav_count == 2) {
                driver = "wavex";
            }
            wav_count++;
        }

        if (strcmp(driver, "mat") == 0) {
            if (mat_count == 0) {
                driver = "mat4";
            } else if (mat_count == 1) {
                driver = "mat5";
            }
            mat_count++;
        }

        if (!driver_list.push_back(DriverInfo(driver, DriverType_File,
                                              DriverFlag_IsDefault
                                                  | DriverFlag_SupportsSource
                                                  | DriverFlag_SupportsSink,
                                              this))) {
            roc_panic("sndfile backend: can't add driver");
        }
    }
}

IDevice* SndfileBackend::open_device(DeviceType device_type,
                                     DriverType driver_type,
                                     const char* driver,
                                     const char* path,
                                     const Config& config,
                                     core::IArena& arena) {
    if (driver_type != DriverType_File) {
        roc_log(LogDebug, "sndfile backend: driver=%s is not a file type", driver);
        return NULL;
    }

    first_created_ = true;

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<SndfileSink> sink(new (arena) SndfileSink(arena, config), arena);
        if (!sink || !sink->is_valid()) {
            roc_log(LogDebug, "sndfile backend: can't construct sink: driver=%s path=%s",
                    driver, path);
            return NULL;
        }

        if (!sink->open(driver, path)) {
            roc_log(LogDebug, "sndfile backend: open failed: driver=%s path=%s", driver,
                    path);
            return NULL;
        }

        return sink.release();
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<SndfileSource> source(new (arena) SndfileSource(arena, config),
                                              arena);
        if (!source || !source->is_valid()) {
            roc_log(LogDebug,
                    "sndfile backend: can't construct source: driver=%s path=%s", driver,
                    path);
            return NULL;
        }

        if (!source->open(driver, path)) {
            roc_log(LogDebug, "sndfile backend: open failed: driver=%s path=%s", driver,
                    path);
            return NULL;
        }

        return source.release();
    } break;

    default:
        break;
    }

    roc_panic("sndfile backend: invalid device type");
}

} // namespace sndio

} // namespace roc
