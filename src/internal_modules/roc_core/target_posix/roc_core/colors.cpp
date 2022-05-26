/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "roc_core/colors.h"

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

bool colors_available() {
    char* term = getenv("TERM");
    return isatty(STDERR_FILENO) && term && strncmp("dumb", term, 4) != 0;
}

bool colors_format(Color color, const char* str, char* buf, size_t bufsz) {
    roc_panic_if_not(str);
    roc_panic_if_not(buf);
    int printed = snprintf(buf, bufsz, "%s%s%s", color_code(color), str, COLOR_RESET);
    return printed > 0 && (size_t)printed < bufsz;
}

} // namespace core
} // namespace roc
