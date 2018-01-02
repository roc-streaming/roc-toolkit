/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdarg.h>
#include <stdio.h>

#include "roc_core/lock.h"
#include "roc_core/log.h"

namespace roc {
namespace core {

Logger::Logger()
    : level_(LogError)
    , handler_(NULL) {
}

LogLevel Logger::level() {
    Lock lock(mutex_);

    return level_;
}

void Logger::set_level(LogLevel level) {
    Lock lock(mutex_);

    if ((int)level < LogNone) {
        level = LogNone;
    }

    if ((int)level > LogTrace) {
        level = LogTrace;
    }

    level_ = level;
}

void Logger::set_handler(LogHandler handler) {
    Lock lock(mutex_);

    handler_ = handler;
}

void Logger::print(const char* module, LogLevel level, const char* format, ...) {
    Lock lock(mutex_);

    if (level > level_ || level == LogNone) {
        return;
    }

    char message[256] = {};

    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message) - 1, format, args);
    va_end(args);

    if (handler_) {
        handler_(level, module, message);
    } else {
        const char* prefix = "?";

        switch (level) {
        case LogNone:
            break;
        case LogError:
            prefix = "error";
            break;
        case LogInfo:
            prefix = "info";
            break;
        case LogDebug:
            prefix = "debug";
            break;
        case LogTrace:
            prefix = "trace";
            break;
        }

        fprintf(stderr, "[%s] %s: %s\n", prefix, module, message);
    }
}

} // namespace core
} // namespace roc
