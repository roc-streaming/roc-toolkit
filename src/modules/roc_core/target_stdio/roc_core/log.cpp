/*
 * Copyright (c) 2015 Roc authors
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
    , colors_(DefaultColorsMode) {
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

void Logger::set_handler(LogHandler handler) {
    Mutex::Lock lock(mutex_);

    handler_ = handler;
}

void Logger::set_colors(ColorsMode colors) {
    Mutex::Lock lock(mutex_);

    colors_ = colors;
}

void Logger::print(const char* module, LogLevel level, const char* format, ...) {
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
        handler_(level, module, message);
    } else {
        default_print_(level, module, message);
    }
}

void Logger::default_print_(LogLevel level, const char* module, const char* message) {
    char timestamp[64] = {};
    if (!format_time(timestamp, sizeof(timestamp))) {
        timestamp[0] = '\0';
    }

    char tid[32] = {};
    if (!format_tid(tid, sizeof(tid))) {
        tid[0] = '\0';
    }

    const char* level_str = "???";
    switch (level) {
    case LogNone:
        break;
    case LogError:
        level_str = "err";
        break;
    case LogInfo:
        level_str = "inf";
        break;
    case LogDebug:
        level_str = "dbg";
        break;
    case LogTrace:
        level_str = "trc";
        break;
    }

    if (colors_ == ColorsEnabled) {
        char colored_level_str[16] = {};
        char colored_message[256] = {};

        if (format_colored(level, level_str, colored_level_str, sizeof(colored_level_str))
            && format_colored(level, message, colored_message, sizeof(colored_message))) {
            fprintf(stderr, "%s [%s] [%s] %s: %s\n", timestamp, tid, colored_level_str,
                    module, colored_message);
            return;
        }
    }

    fprintf(stderr, "%s [%s] [%s] %s: %s\n", timestamp, tid, level_str, module, message);
}

} // namespace core
} // namespace roc
