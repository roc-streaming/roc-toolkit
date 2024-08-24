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
#include "roc_sndio/sndfile_sink.h"
#include "roc_sndio/sndfile_source.h"
#include "roc_sndio/sndfile_tables.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

SndfileBackend::SndfileBackend() {
    roc_log(LogDebug, "sndfile backend: initializing");
}

const char* SndfileBackend::name() const {
    return "sndfile";
}

bool SndfileBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& result) {
    if (!result.push_back(DriverInfo(
            "file", Driver_File | Driver_SupportsSink | Driver_SupportsSource, this))) {
        return false;
    }
    return true;
}

bool SndfileBackend::discover_formats(core::Array<FormatInfo, MaxFormats>& result) {
    SF_FORMAT_INFO format_info;
    int major_format_count = 0;

    if (int err = sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_format_count,
                             sizeof(int))) {
        roc_log(LogError,
                "sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR_COUNT) failed: %s",
                sf_error_number(err));
        return false;
    }

    for (int fmt_index = 0; fmt_index < major_format_count; fmt_index++) {
        format_info.format = fmt_index;
        if (int err = sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info,
                                 sizeof(format_info))) {
            roc_log(LogError,
                    "sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR) failed: %s",
                    sf_error_number(err));
            return false;
        }

        // Format name = file extension.
        const char* format_name = format_info.extension;

        for (size_t map_index = 0; map_index < ROC_ARRAY_SIZE(sndfile_format_remap);
             map_index++) {
            if ((sndfile_format_remap[map_index].format_mask & SF_FORMAT_TYPEMASK)
                == format_info.format) {
                // Some format names are remapped.
                format_name = sndfile_format_remap[map_index].name;
            }
        }

        if (!result.push_back(FormatInfo(
                "file", format_name,
                Driver_File | Driver_SupportsSource | Driver_SupportsSink, this))) {
            roc_log(LogError, "sndfile backend: allocation failed");
            return false;
        }
    }

    return true;
}

bool SndfileBackend::discover_subformat_groups(core::StringList& result) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(sndfile_subformat_map); n++) {
        if (result.find(sndfile_subformat_map[n].group)) {
            continue;
        }
        if (!result.push_back(sndfile_subformat_map[n].group)) {
            roc_log(LogError, "sndfile backend: allocation failed");
            return false;
        }
    }

    return true;
}

bool SndfileBackend::discover_subformats(const char* group, core::StringList& result) {
    roc_panic_if(!group);

    for (size_t n = 0; n < ROC_ARRAY_SIZE(sndfile_subformat_map); n++) {
        if (strcmp(sndfile_subformat_map[n].group, group) != 0) {
            continue;
        }
        if (result.find(sndfile_subformat_map[n].name)) {
            continue;
        }
        if (!result.push_back(sndfile_subformat_map[n].name)) {
            roc_log(LogError, "sndfile backend: allocation failed");
            return false;
        }
    }

    return true;
}

status::StatusCode SndfileBackend::open_device(DeviceType device_type,
                                               const char* driver,
                                               const char* path,
                                               const IoConfig& io_config,
                                               audio::FrameFactory& frame_factory,
                                               core::IArena& arena,
                                               IDevice** result) {
    roc_panic_if(!driver);
    roc_panic_if(!path);

    if (strcmp(driver, "file") != 0) {
        // Not file://, go to next backend.
        return status::StatusNoDriver;
    }

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<SndfileSink> sink(
            new (arena) SndfileSink(frame_factory, arena, io_config, path));

        if (!sink) {
            roc_log(LogDebug, "sndfile backend: can't allocate sink: path=%s", path);
            return status::StatusNoMem;
        }

        if (sink->init_status() != status::StatusOK) {
            roc_log(LogDebug, "sndfile backend: can't open sink: path=%s status=%s", path,
                    status::code_to_str(sink->init_status()));
            return sink->init_status();
        }

        *result = sink.hijack();
        return status::StatusOK;
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<SndfileSource> source(
            new (arena) SndfileSource(frame_factory, arena, io_config, path));

        if (!source) {
            roc_log(LogDebug, "sndfile backend: can't allocate source: path=%s", path);
            return status::StatusNoMem;
        }

        if (source->init_status() != status::StatusOK) {
            roc_log(LogDebug, "sndfile backend: can't open source: path=%s status=%s",
                    path, status::code_to_str(source->init_status()));
            return source->init_status();
        }

        *result = source.hijack();
        return status::StatusOK;
    } break;
    }

    roc_panic("sndfile backend: invalid device type");
}

} // namespace sndio
} // namespace roc
