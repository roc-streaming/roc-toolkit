/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_backend.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/sndfile_extension_table.h"
#include "roc_sndio/sndfile_sink.h"
#include "roc_sndio/sndfile_source.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

SndfileBackend::SndfileBackend() {
    roc_log(LogDebug, "sndfile backend: initializing");
}

const char* SndfileBackend::name() const {
    return "sndfile";
}

void SndfileBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list) {
    SF_FORMAT_INFO format_info;
    int total_number_of_drivers = 0;

    if (int err = sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &total_number_of_drivers,
                             sizeof(int))) {
        roc_panic("sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR_COUNT) failed: %s",
                  sf_error_number(err));
    }

    for (int format_index = 0; format_index < total_number_of_drivers; format_index++) {
        format_info.format = format_index;
        if (int err = sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info,
                                 sizeof(format_info))) {
            roc_panic("sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR) failed: %s",
                      sf_error_number(err));
        }

        const char* driver = format_info.extension;

        for (size_t map_index = 0; map_index < ROC_ARRAY_SIZE(file_type_map);
             map_index++) {
            if (file_type_map[map_index].format_id == format_info.format) {
                driver = file_type_map[map_index].driver_name;
            }
        }

        if (!driver_list.push_back(
                DriverInfo(driver, DriverType_File,
                           DriverFlag_SupportsSource | DriverFlag_SupportsSink, this))) {
            roc_panic("sndfile backend: allocation failed");
        }
    }
}

status::StatusCode SndfileBackend::open_device(DeviceType device_type,
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

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<SndfileSink> sink(
            new (arena) SndfileSink(frame_factory, arena, config), arena);

        if (!sink) {
            roc_log(LogDebug, "sndfile backend: can't allocate sink: driver=%s path=%s",
                    driver, path);
            return status::StatusNoMem;
        }

        if (sink->init_status() != status::StatusOK) {
            roc_log(LogDebug,
                    "sndfile backend: can't initialize sink: driver=%s path=%s status=%s",
                    driver, path, status::code_to_str(sink->init_status()));
            return sink->init_status();
        }

        const status::StatusCode code = sink->open(driver, path);
        if (code != status::StatusOK) {
            roc_log(LogDebug,
                    "sndfile backend: can't open sink: driver=%s path=%s status=%s",
                    driver, path, status::code_to_str(code));
            return code;
        }

        *result = sink.release();
        return status::StatusOK;
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<SndfileSource> source(
            new (arena) SndfileSource(frame_factory, arena, config), arena);

        if (!source) {
            roc_log(LogDebug, "sndfile backend: can't allocate source: driver=%s path=%s",
                    driver, path);
            return status::StatusNoMem;
        }

        if (source->init_status() != status::StatusOK) {
            roc_log(
                LogDebug,
                "sndfile backend: can't initialize source: driver=%s path=%s status=%s",
                driver, path, status::code_to_str(source->init_status()));
            return source->init_status();
        }

        const status::StatusCode code = source->open(driver, path);
        if (code != status::StatusOK) {
            roc_log(LogDebug,
                    "sndfile backend: can't open source: driver=%s path=%s status=%s",
                    driver, path, status::code_to_str(code));
            return code;
        }

        *result = source.release();
        return status::StatusOK;
    } break;

    default:
        break;
    }

    roc_panic("sndfile backend: invalid device type");
}

} // namespace sndio
} // namespace roc
