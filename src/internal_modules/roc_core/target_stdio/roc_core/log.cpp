/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdarg.h>
#include <stdio.h>

#include "roc_core/colors.h"
#include "roc_core/format_tid.h"
#include "roc_core/format_time.h"
#include "roc_core/log.h"

namespace roc {
namespace core {

Logger::Logger()
    : level_(DefaultLogLevel)
    , handler_(NULL)
    , colors_(DefaultColorsMode)
    , location_(DefaultLocationMode) {
}

void Logger::set_verbosity(unsigned verb) {
    switch (verb) {
    case 0:
        set_level(LogError);
        set_location(LocationDisabled);
        break;

    case 1:
        set_level(LogInfo);
        set_location(LocationDisabled);
        break;

    case 2:
        set_level(LogDebug);
        set_location(LocationDisabled);
        break;

    case 3:
        set_level(LogDebug);
        set_location(LocationEnabled);
        break;

    default:
        set_level(LogTrace);
        set_location(LocationEnabled);
        break;
    }
}

void Logger::set_level(LogLevel level) {
    Mutex::Lock lock(mutex_);

    if ((int)level < LogNone) {
        level = LogNone;
    }

    if ((int)level > LogTrace) {
        level = LogTrace;
    }

    AtomicOps::store_relaxed(level_, level);
}

void Logger::set_location(LocationMode location) {
    Mutex::Lock lock(mutex_);

    location_ = location;
}

void Logger::set_colors(ColorsMode colors) {
    Mutex::Lock lock(mutex_);

    colors_ = colors;
}

void Logger::set_handler(LogHandler handler) {
    Mutex::Lock lock(mutex_);

    handler_ = handler;
}

void Logger::print(LogLevel level,
                   const char* module,
                   const char* file,
                   int line,
                   const char* format,
                   ...) {
    Mutex::Lock lock(mutex_);

    if (level > level_ || level == LogNone) {
        return;
    }

    char message[256] = {};
    va_list args;
    va_start(args, format);
    if (vsnprintf(message, sizeof(message) - 1, format, args) < 0) {
        message[0] = '\0';
    }
    va_end(args);

    if (handler_) {
        handler_(level, module, file, line, message);
    } else {
        default_print_(level, module, file, line, message);
    }
}

void Logger::default_print_(LogLevel level_num,
                            const char* module,
                            const char* file,
                            int line,
                            const char* message) {
    char timestamp[64] = {};
    if (!format_time(timestamp, sizeof(timestamp))) {
        timestamp[0] = '\0';
    }

    char tid[32] = {};
    if (!format_tid(tid, sizeof(tid))) {
        tid[0] = '\0';
    }

    const char* level = "???";
    switch (level_num) {
    case LogNone:
        break;
    case LogError:
        level = "err";
        break;
    case LogInfo:
        level = "inf";
        break;
    case LogDebug:
        level = "dbg";
        break;
    case LogTrace:
        level = "trc";
        break;
    }

    if (const char* filename = strrchr(file, '/')) {
        file = filename + 1;
    }

    const char* location = "";
    char location_buf[64] = {};

    if (location_ == LocationEnabled) {
        snprintf(location_buf, sizeof(location_buf), "[%s:%d] ", file, line);
        location = location_buf;
    }

    char colored_level[16] = {};
    char colored_message[256] = {};
    char colored_location[64] = {};

    if (colors_ == ColorsEnabled) {
        Color color = Color_None;

        switch (level_num) {
        case LogError:
            color = Color_Red;
            break;
        case LogInfo:
            color = Color_Blue;
            break;
        default:
            break;
        }

        if (colors_format(color, level, colored_level, sizeof(colored_level))
            && colors_format(color, message, colored_message, sizeof(colored_message))
            && colors_format(Color_Gray, location, colored_location,
                             sizeof(colored_location))) {
            level = colored_level;
            message = colored_message;
            location = colored_location;
        }
    }

    fprintf(stderr, "%s [%s] [%s] %s: %s%s\n", timestamp, tid, level, module, location,
            message);
}

} // namespace core
} // namespace roc
