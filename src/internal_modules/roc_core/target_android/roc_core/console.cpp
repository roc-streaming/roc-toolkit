/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/console.h"
#include "roc_core/stddefs.h"

#include <android/log.h>

namespace roc {
namespace core {

bool console_supports_colors() {
    return false;
}

void console_println(const char* format, ...) {
    va_list args;
    va_start(args, format);

    __android_log_vprint(ANDROID_LOG_DEBUG, "roc", format, args);

    va_end(args);
}

void console_println(Color color, const char* format, ...) {
    va_list args;
    va_start(args, format);

    __android_log_vprint(ANDROID_LOG_DEBUG, "roc", format, args);

    va_end(args);
}

} // namespace core
} // namespace roc
