/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log_backend.h"
#include "roc_core/console.h"
#include "roc_core/log.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

LogBackend::LogBackend() {
    colors_supported_ = console_supports_colors();
}

void LogBackend::handle(const LogMessage& msg) {
    const std::tm tm = nanoseconds_2_tm(msg.time);
    char timestamp_hi[64] = {};
    if (strftime(timestamp_hi, sizeof(timestamp_hi), "%H:%M:%S", &tm) == 0) {
        timestamp_hi[0] = '\0';
    }
    char timestamp_lo[32] = {};
    if (snprintf(timestamp_lo, sizeof(timestamp_lo), "%03lu",
                 (unsigned long)(msg.time % Second / Millisecond))
        <= 0) {
        timestamp_lo[0] = '\0';
    }

    char tid[32] = {};
    if (snprintf(tid, sizeof(tid), "%llu", (unsigned long long)msg.tid) <= 0) {
        tid[0] = '\0';
    }

    const char* level = "???";
    switch (msg.level) {
    case LogNone:
        break;
    case LogError:
        level = "err";
        break;
    case LogInfo:
        level = "inf";
        break;
    case LogNote:
        level = "nte";
        break;
    case LogDebug:
        level = "dbg";
        break;
    case LogTrace:
        level = "trc";
        break;
    }

    char location[64] = {};
    if (msg.location_mode == LocationEnabled && msg.file != NULL) {
        const char* file = msg.file;
        if (const char* filename = strrchr(msg.file, '/')) {
            file = filename + 1;
        }
        if (const char* filename = strrchr(msg.file, '\\')) {
            file = filename + 1;
        }
        snprintf(location, sizeof(location), "[%s:%d] ", file, msg.line);
    }

    Color color = Color_None;
    if (msg.colors_mode == ColorsEnabled
        || (msg.colors_mode == ColorsAuto && colors_supported_)) {
        switch (msg.level) {
        case LogError:
            color = Color_Red;
            break;
        case LogInfo:
            color = Color_Blue;
            break;
        case LogNote:
            color = Color_Green;
            break;
        default:
            break;
        }
    }

    console_println(color, "%s.%s [%s] [%s] %s: %s%s", timestamp_hi, timestamp_lo, tid,
                    level, msg.module, location, msg.text);
}

} // namespace core
} // namespace roc
