/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/scoped_lock.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/sox_backend.h"
#include "roc_sndio/sox_sink.h"
#include "roc_sndio/sox_source.h"

namespace roc {
namespace sndio {

namespace {

const char* default_drivers[] = {
    // sorted in order of priority
    "waveaudio",  // windows
    "coreaudio",  // macos
    "pulseaudio", // linux
    "alsa",       // linux
    "sndio",      // openbsd
    "sunau",      // solaris
    "oss",        // unix
    "ao",         // cross-platform fallback, no capture
};

const char* driver_renames[][2] = {
    { "waveaudio", "wave" },
    { "coreaudio", "core" },
    { "pulseaudio", "pulse" },
};

const char* hidden_drivers[] = {
    // this format doesn't specify the encoding explicitly
    // use its explicit variants like f32, s32, etc
    "raw",
    // deprecated aliases
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
    // pseudo-formats
    "sndfile",
};

bool is_default(const char* driver) {
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

bool check_handler_flags(const sox_format_handler_t*& handler, int filter_flags) {
    if (!handler) {
        return false;
    }

    if (handler->flags & SOX_FILE_DEVICE) {
        if (handler->flags & SOX_FILE_PHONY) {
            return false;
        }

        if ((filter_flags & IBackend::FilterDevice) == 0) {
            return false;
        }
    } else {
        if ((filter_flags & IBackend::FilterFile) == 0) {
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
    case 1: // fail
        level = LogError;
        break;

    case 2: // warn
        level = LogInfo;
        break;

    case 3: // info
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

void add_driver_mapping(core::Array<DriverInfo>& list,
                        const char* driver,
                        IBackend* backend,
                        unsigned int driver_flags) {
    for (size_t n = 0; n < list.size(); n++) {
        if (strcmp(list[n].name, driver) == 0 && list[n].backend == backend) {
            return;
        }
    }
    DriverInfo driver_info;
    driver_info.set(driver, backend, driver_flags);
    list.push_back(driver_info);
}

template <class T>
bool check_and_open(const char* driver,
                    const char* inout,
                    int filter_flags,
                    const core::ScopedPtr<T>& sinksource) {
    if (driver && is_driver_hidden(driver)) {
        roc_log(LogDebug, "driver is not supported");
        return false;
    }

    const sox_format_handler_t* handler = sox_write_handler(inout, driver, NULL);
    if (!check_handler_flags(handler, filter_flags)) {
        return false;
    }

    if (!sinksource) {
        return false;
    }

    if (!sinksource->valid()) {
        return false;
    }

    if (!sinksource->open(driver, inout)) {
        roc_log(LogDebug, "sox: driver open failed");
        return false;
    }

    return true;
}

} // namespace

SoxBackend::SoxBackend()
    : first_created_(false) {
    roc_log(LogDebug, "initializing sox backend");

    sox_init();

    sox_get_globals()->verbosity = 100;
    sox_get_globals()->output_message_handler = log_handler;
}

void SoxBackend::set_frame_size(core::nanoseconds_t frame_length,
                                size_t sample_rate,
                                packet::channel_mask_t channels) {
    core::Mutex::Lock lock(mutex_);

    size_t size = packet::ns_to_size(frame_length, sample_rate, channels);

    if (first_created_) {
        roc_panic("sox backend: set_frame_size() can be called only before creating "
                  "first source or sink");
    }

    sox_get_globals()->bufsiz = size * sizeof(sox_sample_t);
}

ISink* SoxBackend::open_sink(core::IAllocator& allocator,
                             const char* driver,
                             const char* output,
                             const Config& config,
                             int filter_flags) {
    core::Mutex::Lock lock(mutex_);
    first_created_ = true;

    driver = map_to_sox_driver(driver);
    core::ScopedPtr<SoxSink> sink(new (allocator) SoxSink(allocator, config), allocator);
    if (check_and_open(driver, output, filter_flags, sink)) {
        return sink.release();
    }
    return NULL;
}

ISource* SoxBackend::open_source(core::IAllocator& allocator,
                                 const char* driver,
                                 const char* input,
                                 const Config& config,
                                 int filter_flags) {
    core::Mutex::Lock lock(mutex_);

    first_created_ = true;

    driver = map_to_sox_driver(driver);
    core::ScopedPtr<SoxSource> source(new (allocator) SoxSource(allocator, config),
                                      allocator);
    if (check_and_open(driver, input, filter_flags, source)) {
        return source.release();
    }

    return NULL;
}

bool SoxBackend::get_drivers(core::Array<DriverInfo>& list, int filter_flags) {
    core::Mutex::Lock lock(mutex_);

    for (size_t n = 0; n < ROC_ARRAY_SIZE(default_drivers); n++) {
        const char* driver = map_from_sox_driver(default_drivers[n]);
        add_driver_mapping(list, driver, this,
                           DriverDevice | DriverDefault | DriverSource | DriverSink);
    }

    const sox_format_tab_t* formats = sox_get_format_fns();

    for (size_t n = 0; formats[n].fn; n++) {
        sox_format_handler_t const* handler = formats[n].fn();

        if (check_handler_flags(handler, filter_flags)) {
            char const* const* format_names;
            for (format_names = handler->names; *format_names; ++format_names) {
                const char* driver = map_from_sox_driver(*format_names);

                if (is_driver_hidden(driver) || is_default(driver)) {
                    continue;
                }

                unsigned int driver_flags = DriverSource | DriverSink;
                if (!(handler->flags & SOX_FILE_DEVICE)) {
                    driver_flags |= DriverFile;

                } else {
                    driver_flags |= DriverDevice;
                }
                add_driver_mapping(list, driver, this, driver_flags);
            }
        }
    }

    return true;
}

} // namespace sndio
} // namespace roc
