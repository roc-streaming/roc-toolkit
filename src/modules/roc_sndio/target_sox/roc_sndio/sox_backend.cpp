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

const char* default_driver_priorities[] = {
    //
    "waveaudio",  // windows
    "coreaudio",  // macos
    "pulseaudio", // linux
    "alsa",       // linux
    "sndio",      // openbsd
    "sunau",      // solaris
    "oss",        // unix
    "ao",         // cross-platform fallback, no capture
    "null"        //
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

const char* select_default_driver() {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(default_driver_priorities); n++) {
        const char* driver = default_driver_priorities[n];

        if (sox_find_format(driver, sox_false)) {
            return driver;
        }
    }

    return NULL;
}

const char* select_default_device(const char* driver) {
    const sox_format_handler_t* format = sox_find_format(driver, sox_false);
    if (!format) {
        return NULL;
    }

    if (format->flags & SOX_FILE_DEVICE) {
        return "default";
    }

    return "-";
}

bool select_defaults(const char*& driver, const char*& device) {
    if (!device) {
        if (!driver) {
            if (!(driver = select_default_driver())) {
                return false;
            }
        }
        if (!(device = select_default_device(driver))) {
            return false;
        }
    }
    return true;
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

    if (level > core::Logger::instance().level()) {
        return;
    }

    char message[256] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);

    roc_log(level, "sox: %s: %s", filename, message);
}

} // namespace

SoxBackend::SoxBackend()
    : first_created_(false) {
    roc_log(LogDebug, "initializing sox backend");

    sox_init();

    sox_get_globals()->verbosity = 100;
    sox_get_globals()->output_message_handler = log_handler;
}

void SoxBackend::set_frame_size(size_t size) {
    core::Mutex::Lock lock(mutex_);

    if (first_created_) {
        roc_panic("sox backend: set_frame_size() can be called only before creating "
                  "first source or sink");
    }

    sox_get_globals()->bufsiz = size * sizeof(sox_sample_t);
}

bool SoxBackend::probe(const char* driver, const char* inout, int filter_flags) {
    core::Mutex::Lock lock(mutex_);

    driver = map_to_sox_driver(driver);

    if (!select_defaults(driver, inout)) {
        return false;
    }

    const sox_format_handler_t* handler = sox_write_handler(inout, driver, NULL);
    if (!handler) {
        return false;
    }

    if (handler->flags & SOX_FILE_DEVICE) {
        if ((filter_flags & FilterDevice) == 0) {
            return false;
        }
    } else {
        if ((filter_flags & FilterFile) == 0) {
            return false;
        }
    }

    return true;
}

ISink* SoxBackend::open_sink(core::IAllocator& allocator,
                             const char* driver,
                             const char* output,
                             const Config& config) {
    core::Mutex::Lock lock(mutex_);

    first_created_ = true;

    driver = map_to_sox_driver(driver);

    if (!select_defaults(driver, output)) {
        return NULL;
    }

    core::ScopedPtr<SoxSink> sink(new (allocator) SoxSink(allocator, config), allocator);
    if (!sink) {
        return NULL;
    }

    if (!sink->valid()) {
        return NULL;
    }

    if (!sink->open(driver, output)) {
        return NULL;
    }

    return sink.release();
}

ISource* SoxBackend::open_source(core::IAllocator& allocator,
                                 const char* driver,
                                 const char* input,
                                 const Config& config) {
    core::Mutex::Lock lock(mutex_);

    first_created_ = true;

    driver = map_to_sox_driver(driver);

    if (!select_defaults(driver, input)) {
        return NULL;
    }

    core::ScopedPtr<SoxSource> source(new (allocator) SoxSource(allocator, config),
                                      allocator);
    if (!source) {
        return NULL;
    }

    if (!source->valid()) {
        return NULL;
    }

    if (!source->open(driver, input)) {
        return NULL;
    }

    return source.release();
}

bool SoxBackend::get_drivers(core::StringList& list, int filter_flags) {
    core::Mutex::Lock lock(mutex_);

    const sox_format_tab_t* formats = sox_get_format_fns();

    for (size_t n = 0; formats[n].fn; n++) {
        sox_format_handler_t const* handler = formats[n].fn();

        bool match = false;

        if (filter_flags & FilterFile) {
            match = match || !(handler->flags & SOX_FILE_DEVICE);
        }

        if (filter_flags & FilterDevice) {
            match = match
                || ((handler->flags & SOX_FILE_DEVICE)
                    && !(handler->flags & SOX_FILE_PHONY));
        }

        if (match) {
            char const* const* format_names;
            for (format_names = handler->names; *format_names; ++format_names) {
                const char* driver = map_from_sox_driver(*format_names);

                if (is_driver_hidden(driver)) {
                    continue;
                }

                if (!list.push_back_uniq(driver)) {
                    return false;
                }
            }
        }
    }

    return true;
}

} // namespace sndio
} // namespace roc
