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
    { "waveaudio", "wave" },
    { "coreaudio", "core" },
};

const char* hidden_drivers[] = {
    // raw format aliases
    "f4",
    "f8",
    "s1",
    "s2",
    "s3",
    "s4",
    "u1",
    "u2",
    "u3",
    "u4",
    "sb",
    "sw",
    "sl",
    "ub",
    "uw",
    "f32",
    "f64",
    "s8",
    "s16",
    "s24",
    "s32",
    "u8",
    "u16",
    "u24",
    "u32",
    // formats already handled by libsndfile
    "aif",
    "aifc",
    "aiff",
    "aiffc",
    "mat",
    "mat4",
    "mat5",
    "vorbis",
    // pseudo-formats
    "sndfile",
    "null",
    "wavpcm",
    // unsupported device drivers
    "ao",
    "ossdsp",
    "pulseaudio",
};

bool is_default_driver(const char* driver) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(default_drivers); n++) {
        if (strcmp(driver, default_drivers[n]) == 0) {
            return true;
        }
    }

    return false;
}

const char* map_to_sox_driver(const char* driver) {
    if (!driver) {
        return NULL;
    }
    for (size_t n = 0; n < ROC_ARRAY_SIZE(driver_renames); n++) {
        if (strcmp(driver_renames[n][1], driver) == 0) {
            return driver_renames[n][0];
        }
    }
    return driver;
}

const char* map_from_sox_driver(const char* driver) {
    if (!driver) {
        return NULL;
    }
    for (size_t n = 0; n < ROC_ARRAY_SIZE(driver_renames); n++) {
        if (strcmp(driver_renames[n][0], driver) == 0) {
            return driver_renames[n][1];
        }
    }
    return driver;
}

bool is_driver_hidden(const char* driver) {
    // replicate the behavior of display_supported_formats() from sox.c
    if (strchr(driver, '/')) {
        return true;
    }
    for (size_t n = 0; n < ROC_ARRAY_SIZE(hidden_drivers); n++) {
        if (strcmp(hidden_drivers[n], driver) == 0) {
            return true;
        }
    }
    return false;
}

bool check_handler_type(const sox_format_handler_t* handler, DriverType driver_type) {
    if (handler->flags & SOX_FILE_DEVICE) {
        if (handler->flags & SOX_FILE_PHONY) {
            return false;
        }
        if (driver_type != DriverType_Device) {
            return false;
        }
    } else {
        if (driver_type != DriverType_File) {
            return false;
        }
    }

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

SoxBackend::SoxBackend()
    : first_created_(false) {
    sox_init();

    sox_get_globals()->verbosity = 100;
    sox_get_globals()->output_message_handler = log_handler;
}

void SoxBackend::set_frame_size(core::nanoseconds_t frame_length,
                                const audio::SampleSpec& sample_spec) {
    size_t size = sample_spec.ns_2_samples_overall(frame_length);

    if (first_created_) {
        roc_panic(
            "sox backend:"
            " set_frame_size() can be called only before creating first source or sink");
    }

    sox_get_globals()->bufsiz = size * sizeof(sox_sample_t);
}

const char* SoxBackend::name() const {
    return "sox";
}

void SoxBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(default_drivers); n++) {
        const sox_format_handler_t* handler =
            sox_write_handler(NULL, default_drivers[n], NULL);
        if (!handler) {
            continue;
        }

        const char* driver = map_from_sox_driver(default_drivers[n]);

        if (!driver_list.push_back(DriverInfo(driver, DriverType_Device,
                                              DriverFlag_IsDefault
                                                  | DriverFlag_SupportsSource
                                                  | DriverFlag_SupportsSink,
                                              this))) {
            roc_panic("sox backend: can't add driver");
        }
    }

    const sox_format_tab_t* formats = sox_get_format_fns();

    for (size_t n = 0; formats[n].fn; n++) {
        sox_format_handler_t const* handler = formats[n].fn();

        char const* const* format_names;
        for (format_names = handler->names; *format_names; ++format_names) {
            const char* driver = map_from_sox_driver(*format_names);

            if (is_driver_hidden(driver) || is_default_driver(driver)) {
                continue;
            }

            if (!driver_list.push_back(DriverInfo(
                    driver,
                    (handler->flags & SOX_FILE_DEVICE) ? DriverType_Device
                                                       : DriverType_File,
                    DriverFlag_SupportsSource | DriverFlag_SupportsSink, this))) {
                roc_panic("sox backend: can't add driver");
            }
        }
    }
}

status::StatusCode SoxBackend::open_device(DeviceType device_type,
                                           DriverType driver_type,
                                           const char* driver,
                                           const char* path,
                                           const IoConfig& io_config,
                                           audio::FrameFactory& frame_factory,
                                           core::IArena& arena,
                                           IDevice** result) {
    first_created_ = true;

    driver = map_to_sox_driver(driver);

    if (driver && is_driver_hidden(driver)) {
        roc_log(LogDebug, "sox backend: driver is not supported: driver=%s path=%s",
                driver, path);
        return status::StatusNoDriver;
    }

    const sox_format_handler_t* handler = sox_write_handler(path, driver, NULL);
    if (!handler) {
        roc_log(LogDebug, "sox backend: driver is not available: driver=%s path=%s",
                driver, path);
        return status::StatusNoDriver;
    }

    if (!check_handler_type(handler, driver_type)) {
        roc_log(LogDebug, "sox backend: mismatching driver type: driver=%s path=%s",
                driver, path);
        return status::StatusNoDriver;
    }

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<SoxSink> sink(
            new (arena) SoxSink(frame_factory, arena, io_config, driver_type));

        if (!sink) {
            roc_log(LogDebug, "sox backend: can't allocate sink: path=%s", path);
            return status::StatusNoMem;
        }

        if (sink->init_status() != status::StatusOK) {
            roc_log(LogDebug, "sox backend: can't initialize sink: path=%s status=%s",
                    path, status::code_to_str(sink->init_status()));
            return sink->init_status();
        }

        const status::StatusCode code = sink->open(driver, path);
        if (code != status::StatusOK) {
            roc_log(LogDebug, "sox backend: can't open sink: path=%s status=%s", path,
                    status::code_to_str(code));
            return code;
        }

        *result = sink.hijack();
        return status::StatusOK;
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<SoxSource> source(
            new (arena) SoxSource(frame_factory, arena, io_config, driver_type));

        if (!source) {
            roc_log(LogDebug, "sox backend: can't allocate source: path=%s", path);
            return status::StatusNoMem;
        }

        if (source->init_status() != status::StatusOK) {
            roc_log(LogDebug, "sox backend: can't initialize source: path=%s status=%s",
                    path, status::code_to_str(source->init_status()));
            return source->init_status();
        }

        const status::StatusCode code = source->open(driver, path);
        if (code != status::StatusOK) {
            roc_log(LogDebug, "sox backend: can't open source: path=%s status=%s", path,
                    status::code_to_str(code));
            return code;
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
