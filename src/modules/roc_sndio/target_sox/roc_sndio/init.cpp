/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdarg.h>

#include <sox.h>

#include "roc_core/log.h"
#include "roc_sndio/init.h"

namespace roc {
namespace sndio {

namespace {

void message_handler(unsigned slevel,
                     const char* filename,
                     const char* format,
                     va_list args) {
    //
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

    if (level > core::get_log_level()) {
        return;
    }

    char message[256] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);

    roc_log(level, "[sox] %s: %s", filename, message);
}

bool init_done = false;

} // namespace

void init(int options, size_t bufsz) {
    if ((options & InitOnce) && init_done) {
        return;
    }

    if (options & InitSox) {
        roc_log(LogInfo, "initializing sox");
        sox_init();
    }

    if (options & InitLog) {
        sox_get_globals()->verbosity = 100;
        sox_get_globals()->output_message_handler = message_handler;
    }

    if (options & InitBufsz) {
        sox_get_globals()->bufsiz = bufsz;
        sox_get_globals()->input_bufsiz = bufsz;
    }

    init_done = true;
}

} // namespace sndio
} // namespace roc
