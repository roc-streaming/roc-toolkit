/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sox.h>

#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#include "roc_sndio/default.h"
#include "roc_sndio/init.h"

namespace roc {
namespace sndio {

namespace {

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

} // namespace

bool detect_defaults(const char** name, const char** type) {
    roc_panic_if(!name);
    roc_panic_if(!type);

    sndio::init();

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

const char* default_driver() {
    if (driver) {
        return driver;
    }

    sndio::init();

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

} // namespace sndio
} // namespace roc
