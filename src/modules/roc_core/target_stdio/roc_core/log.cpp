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

#include "roc_core/log.h"

namespace roc {
namespace core {

namespace {

LogLevel g_log_level = LogError;
LogHandler g_log_handler = NULL;

} // namespace

LogLevel get_log_level() {
    return g_log_level;
}

LogLevel set_log_level(LogLevel level) {
    if ((int)level < LogNone) {
        level = LogNone;
    }
    if ((int)level > LogTrace) {
        level = LogTrace;
    }
    LogLevel ret = g_log_level;
    g_log_level = level;
    return ret;
}

LogHandler set_log_handler(LogHandler handler) {
    LogHandler ret = g_log_handler;
    g_log_handler = handler;
    return ret;
}

void print_message(const char* module, LogLevel level, const char* format, ...) {
    if (level > g_log_level || level == LogNone) {
        return;
    }

    char message[256] = {};

    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message) - 1, format, args);
    va_end(args);

    if (g_log_handler) {
        g_log_handler(level, module, message);
    } else {
        const char* prefix = "?????";

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
