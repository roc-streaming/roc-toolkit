/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/sox_controller.h"

namespace roc {
namespace sndio {

namespace {

const char* driver_priorities[] = {
    //
    "waveaudio",  // windows
    "coreaudio",  // macos
    "pulseaudio", // linux
    "alsa",       // linux
    "sndio",      // openbsd
    "sunaudio",   // solaris
    "oss",        // unix
    "ao",         // cross-platform fallback, no capture
    "null"        //
};

const char* select_default_driver() {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(driver_priorities); n++) {
        const char* driver = driver_priorities[n];

        if (sox_find_format(driver, sox_false)) {
            roc_log(LogDebug, "selecting default sox driver '%s'", driver);
            return driver;
        }
    }

    roc_log(LogError, "none of the known sox drivers are available");
    return NULL;
}

const char* select_default_device(const char* driver) {
    const sox_format_handler_t* format = sox_find_format(driver, sox_false);
    if (!format) {
        roc_log(LogError, "unrecognized sox driver '%s", driver);
        return NULL;
    }

    if (format->flags & SOX_FILE_DEVICE) {
        return "default";
    }

    return "-";
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

} // namespace

SoxController::SoxController() {
    roc_log(LogInfo, "initializing sox");

    sox_init();

    sox_get_globals()->bufsiz = 8192;
    sox_get_globals()->verbosity = 100;
    sox_get_globals()->output_message_handler = log_handler;
}

sox_globals_t& SoxController::get_globals() const {
    return *sox_get_globals();
}

bool SoxController::fill_defaults(const char*& driver, const char*& device) {
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

} // namespace sndio
} // namespace roc
