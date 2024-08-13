/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/console.h"
#include "roc_core/atomic_ops.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ANSI Color Codes.
#define COLOR_NONE ""
#define COLOR_START "\033["
#define COLOR_END "m"
#define COLOR_RESET COLOR_START "0" COLOR_END
#define COLOR_SEPARATOR ";"
#define COLOR_BOLD "1"
#define COLOR_BLACK "30"
#define COLOR_RED "31"
#define COLOR_GREEN "32"
#define COLOR_YELLOW "33"
#define COLOR_BLUE "34"
#define COLOR_MAGENTA "35"
#define COLOR_CYAN "36"
#define COLOR_WHITE "37"

namespace roc {
namespace core {

namespace {

// Serializes console operations.
pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;

// -1 means unknown, 0 means no support, +1 means have support.
int console_colors = -1;

bool env_has_no_color() {
    const char* no_color = getenv("NO_COLOR");
    if (no_color) {
        return strnlen(no_color, 16) > 0;
    }
    return false;
}

bool env_has_force_color() {
    const char* force_color = getenv("FORCE_COLOR");
    if (force_color) {
        char* end;
        long value = strtol(force_color, &end, 10);
        if (*force_color != '\0' && *end == '\0') {
            return value > 0;
        }
    }
    return false;
}

bool term_supports_color() {
    if (isatty(STDERR_FILENO)) {
        const char* term = getenv("TERM");
        if (term) {
            return strncmp("dumb", term, 4) != 0;
        }
    }
    return false;
}

bool detect_color_support() {
    if (env_has_no_color()) {
        return false;
    } else if (env_has_force_color()) {
        return true;
    }
    return term_supports_color();
}

const char* color_code(Color color) {
    switch (color) {
    case Color_White:
        return COLOR_START COLOR_BOLD COLOR_SEPARATOR COLOR_WHITE COLOR_END;
    case Color_Gray:
        return COLOR_START COLOR_SEPARATOR COLOR_WHITE COLOR_END;
    case Color_Red:
        return COLOR_START COLOR_BOLD COLOR_SEPARATOR COLOR_RED COLOR_END;
    case Color_Green:
        return COLOR_START COLOR_BOLD COLOR_SEPARATOR COLOR_GREEN COLOR_END;
    case Color_Yellow:
        return COLOR_START COLOR_BOLD COLOR_SEPARATOR COLOR_YELLOW COLOR_END;
    case Color_Blue:
        return COLOR_START COLOR_BOLD COLOR_SEPARATOR COLOR_BLUE COLOR_END;
    case Color_Magenta:
        return COLOR_START COLOR_BOLD COLOR_SEPARATOR COLOR_MAGENTA COLOR_END;
    case Color_Cyan:
        return COLOR_START COLOR_BOLD COLOR_SEPARATOR COLOR_CYAN COLOR_END;
    default:
        break;
    }
    return COLOR_NONE;
}

} // namespace

bool console_supports_colors() {
    int colors = AtomicOps::load_seq_cst(console_colors);

    if (colors == -1) {
        pthread_mutex_lock(&console_mutex);

        colors = detect_color_support() ? 1 : 0;
        AtomicOps::store_seq_cst(console_colors, colors);

        pthread_mutex_unlock(&console_mutex);
    }

    return colors;
}

void console_println(const char* format, ...) {
    pthread_mutex_lock(&console_mutex);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);

    pthread_mutex_unlock(&console_mutex);
}

void console_println(Color color, const char* format, ...) {
    const bool use_colors = (color != Color_None) && console_supports_colors();

    pthread_mutex_lock(&console_mutex);

    if (use_colors) {
        fprintf(stderr, "%s", color_code(color));
    }

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    if (use_colors) {
        fprintf(stderr, "%s", COLOR_RESET);
    }

    fprintf(stderr, "\n");
    fflush(stderr);

    pthread_mutex_unlock(&console_mutex);
}

} // namespace core
} // namespace roc
