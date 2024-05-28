/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_android/roc_core/console.h
//! @brief Console functions.

#ifndef ROC_CORE_CONSOLE_H_
#define ROC_CORE_CONSOLE_H_

#include "roc_core/attributes.h"

namespace roc {
namespace core {

//! Color ID.
enum Color {
    Color_None,
};

//! Check if colors can be used.
bool console_supports_colors();

//! Print line.
ROC_ATTR_PRINTF(1, 2) void console_println(const char* format, ...);

//! Print line (with color).
ROC_ATTR_PRINTF(2, 3) void console_println(Color color, const char* format, ...);

} // namespace core
} // namespace roc

#endif // ROC_CORE_CONSOLE_H_
