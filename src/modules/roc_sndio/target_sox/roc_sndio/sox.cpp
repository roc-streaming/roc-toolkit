/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sox.h>
#include <stdio.h>

#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"
#include "roc_sndio/sox.h"

namespace roc {
namespace sndio {

namespace {

void log_handler(unsigned slevel,
                 const char* filename,
                 const char* format,
                 va_list args) {
    LogLevel level;

    switch (slevel) {
    case 0:
    case 1: // fail
        level = LogError;
        break;

    case 2: // warn
    case 3: // info
    case 4: // debug
        level = LogInfo;
        break;

    default: // debug_more, debug most
        level = LogDebug;
        break;
    }

    if (level > core::Logger::instance().level()) {
        return;
    }

    char message[256] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);

    roc_log(level, "[sox] %s: %s", filename, message);
}

const char* driver = NULL;

const char* driver_list[] = {
    //
    "waveaudio",  // windows
    "coreaudio",  // macos
    "pulseaudio", // linux
    "alsa",       // linux
    "sndio",      // openbsd
    "sunaudio",   // solaris
    "oss",        // unix
    "ao",         // cross-platform, no capture
    "null"        //
};

const char* default_driver() {
    if (driver) {
        return driver;
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(driver_list); n++) {
        const char* drv = driver_list[n];

        if (sox_find_format(drv, sox_false)) {
            roc_log(LogDebug, "selecting default driver %s", drv);
            return (driver = drv);
        } else {
            roc_log(LogDebug, "driver %s is not supported", drv);
        }
    }

    roc_log(LogError, "none of the known drivers supported");
    return NULL;
}

const char* default_device() {
    return "default";
}

} // namespace

void sox_setup() {
    if (sox_get_globals()->output_message_handler == log_handler) {
        return;
    }

    roc_log(LogInfo, "initializing sox");
    sox_init();

    sox_get_globals()->verbosity = 100;
    sox_get_globals()->output_message_handler = log_handler;
}

bool sox_defaults(const char** name, const char** type) {
    roc_panic_if(!name);
    roc_panic_if(!type);

    if (*name && *type) {
        return true;
    }

    if (!*name) {
        if (!*type) {
            *name = default_device();
            *type = default_driver();
        } else if (const sox_format_handler_t* format =
                       sox_find_format(*type, sox_false)) {
            if (format->flags & SOX_FILE_DEVICE) {
                *name = default_device();
            } else {
                *name = "-";
            }
        }
    }

    if (!*name) {
        roc_log(LogError, "can't detect default file/device name");
        return false;
    }

    roc_log(LogDebug, "detected defaults: name=%s type=%s", *name, *type);
    return true;
}

} // namespace sndio
} // namespace roc
