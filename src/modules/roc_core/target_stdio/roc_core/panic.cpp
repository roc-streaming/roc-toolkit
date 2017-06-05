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
#include <stdlib.h>

#include "roc_core/panic.h"

namespace roc {
namespace core {

namespace {

enum { PANIC_MAX_MSG = 128 };

PanicHandler g_panic_handler = NULL;

} // namespace

void set_panic_handler(PanicHandler handler) {
    g_panic_handler = handler;
}

void do_panic(const char* module, const char* file, int line, const char* format, ...) {
    char message[PANIC_MAX_MSG] = {};

    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message) - 1, format, args);
    va_end(args);

    if (g_panic_handler) {
        g_panic_handler(message);
    }

    fprintf(stderr, "\n%s:%d: error: roc_panic() called\n\n", file, line);
    fprintf(stderr, "PANIC: %s: %s\n\n", module, message);
    print_backtrace();

    abort();
}

} // namespace core
} // namespace roc
