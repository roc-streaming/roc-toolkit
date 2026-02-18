/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/sox_backend.h"
#include "roc_sndio/sox_sink.h"
#include "roc_sndio/sox_source.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

// sorted in order of priority
const char* default_drivers[] = {
    "waveaudio", // windows
    "coreaudio", // macos
    "alsa",      // linux
    "sndio",     // openbsd
    "oss",       // unix
};

const char* driver_renames[][2] = {
    // device drivers
    { "waveaudio", "wave" },
    { "coreaudio", "core" },
    // file formats
    { "anb", "amr" },
};

const char* hidden_drivers[] = {
    "ao",
    "ossdsp",
    "pulseaudio",
};

const char* driver_to_sox(const char* name) {
    if (!name) {
        return NULL;
    }
    for (size_t n = 0; n < ROC_ARRAY_SIZE(driver_renames); n++) {
        if (strcmp(driver_renames[n][1], name) == 0) {
            return driver_renames[n][0];
        }
    }
    return name;
}

const char* driver_from_sox(const char* name) {
    if (!name) {
        return NULL;
    }
    for (size_t n = 0; n < ROC_ARRAY_SIZE(driver_renames); n++) {
        if (strcmp(driver_renames[n][0], name) == 0) {
            return driver_renames[n][1];
        }
    }
    return name;
}

bool is_default_driver(const char* name) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(default_drivers); n++) {
        if (strcmp(name, default_drivers[n]) == 0) {
            return true;
        }
    }
    return false;
}

bool is_supported_driver(const char* name) {
    const sox_format_handler_t* format_handler = sox_write_handler(NULL, name, NULL);
    if (format_handler == NULL) {
        // not enabled in sox
        return false;
    }
    if (!(format_handler->flags & SOX_FILE_DEVICE)) {
        // not device
        return false;
    }
    if (format_handler->flags & SOX_FILE_PHONY) {
        // phony device
        return false;
    }

    if (strchr(name, '/')) {
        // replicate the behavior of display_supported_formats() from sox.c
        return false;
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(hidden_drivers); n++) {
        // hidden by us
        if (strcmp(hidden_drivers[n], name) == 0) {
            return false;
        }
    }

    // supported!
    return true;
}

void log_handler(unsigned sox_level,
                 const char* filename,
                 const char* format,
                 va_list args) {
    LogLevel level;

    switch (sox_level) {
    case 0:
    case 1:
    case 2:
    case 3: // fail, warn, info
        level = LogDebug;
        break;

    default: // debug, debug more, debug most
        level = LogTrace;
        break;
    }

    if (level > core::Logger::instance().get_level()) {
        return;
    }

    char message[256] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);

    roc_log(level, "sox: %s: %s", filename, message);
}

} // namespace

SoxBackend::SoxBackend() {
    sox_init();

    sox_get_globals()->verbosity = 100;
    sox_get_globals()->output_message_handler = log_handler;
}

const char* SoxBackend::name() const {
    return "sox";
}

bool SoxBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& result) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(default_drivers); n++) {
        const char* driver = default_drivers[n];
        if (!is_supported_driver(driver)) {
            continue;
        }

        if (!result.push_back(DriverInfo(driver_from_sox(driver),
                                         Driver_Device | Driver_DefaultDevice
                                             | Driver_SupportsSource
                                             | Driver_SupportsSink,
                                         this))) {
            return false;
        }
    }

    const sox_format_tab_t* formats = sox_get_format_fns();
    for (size_t n = 0; formats[n].fn; n++) {
        sox_format_handler_t const* format_handler = formats[n].fn();
        char const* const* format_names;

        for (format_names = format_handler->names; *format_names; ++format_names) {
            const char* driver = *format_names;
            if (!is_supported_driver(driver) || is_default_driver(driver)) {
                continue;
            }

            if (!result.push_back(DriverInfo(
                    driver_from_sox(driver),
                    Driver_Device | Driver_SupportsSource | Driver_SupportsSink, this))) {
                return false;
            }
        }
    }

    return true;
}

bool SoxBackend::discover_formats(core::Array<FormatInfo, MaxFormats>& result) {
    // no formats except pcm
    return true;
}

bool SoxBackend::discover_subformat_groups(core::StringList& result) {
    // no sub-formats except pcm
    return true;
}

bool SoxBackend::discover_subformats(const char* group, core::StringList& result) {
    // no sub-formats except pcm
    return true;
}

status::StatusCode SoxBackend::open_device(DeviceType device_type,
                                           const char* driver,
                                           const char* path,
                                           const IoConfig& io_config,
                                           audio::FrameFactory& frame_factory,
                                           core::IArena& arena,
                                           IDevice** result) {
    roc_panic_if(!driver);
    roc_panic_if(!path);

    if (driver) {
        driver = driver_to_sox(driver);

        if (!is_supported_driver(driver)) {
            roc_log(LogDebug,
                    "sox backend sink: requested driver not supported by backend:"
                    " driver=%s path=%s",
                    driver, path);
            // Try another backend.
            return status::StatusNoDriver;
        }
    }

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<SoxSink> sink(
            new (arena) SoxSink(frame_factory, arena, io_config, driver, path));

        if (!sink) {
            roc_log(LogDebug, "sox backend: can't allocate sink: driver=%s path=%s",
                    driver, path);
            return status::StatusNoMem;
        }

        if (sink->init_status() != status::StatusOK) {
            roc_log(LogDebug, "sox backend: can't open sink: driver=%s path=%s status=%s",
                    driver, path, status::code_to_str(sink->init_status()));
            return sink->init_status();
        }

        *result = sink.hijack();
        return status::StatusOK;
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<SoxSource> source(
            new (arena) SoxSource(frame_factory, arena, io_config, driver, path));

        if (!source) {
            roc_log(LogDebug, "sox backend: can't allocate source: driver=%s path=%s",
                    driver, path);
            return status::StatusNoMem;
        }

        if (source->init_status() != status::StatusOK) {
            roc_log(LogDebug,
                    "sox backend: can't open source: driver=%s path=%s status=%s", driver,
                    path, status::code_to_str(source->init_status()));
            return source->init_status();
        }

        *result = source.hijack();
        return status::StatusOK;
    } break;

    default:
        break;
    }

    roc_panic("sox backend: invalid device type");
}

} // namespace sndio
} // namespace roc
