/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/color_print.h
//! @brief Colorization functions.

#ifndef ROC_CORE_COLOR_PRINT_H_
#define ROC_CORE_COLOR_PRINT_H_

#include "roc_core/log.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Color ID.
enum Color {
    Color_None,
    Color_White,
    Color_Gray,
    Color_Red,
    Color_Green,
    Color_Yellow,
    Color_Blue,
    Color_Magenta,
    Color_Cyan
};

//! Check if current stderr is connected to a tty.
bool colors_available();

//! Fill colored str into buf according to the log level.
bool colors_format(Color color, const char* str, char* buf, size_t bufsz);

} // namespace core
} // namespace roc

#endif // ROC_CORE_COLOR_PRINT_H_
