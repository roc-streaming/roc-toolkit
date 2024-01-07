/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_android/roc_core/console.h
//! @brief Console.

#ifndef ROC_CORE_CONSOLE_H_
#define ROC_CORE_CONSOLE_H_

#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"

namespace roc {
namespace core {

//! Color ID.
enum Color {
    Color_None,
};

//! Console.
class Console : public NonCopyable<> {
public:
    //! Get logger instance.
    static Console& instance() {
        return Singleton<Console>::instance();
    }

    //! Check if colors can be used.
    bool colors_supported();

    //! Print line.
    ROC_ATTR_PRINTF(2, 3) void println(const char* format, ...);

    //! Print line.
    ROC_ATTR_PRINTF(3, 4) void println_color(Color color, const char* format, ...);

private:
    friend class Singleton<Console>;

    Console();
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_CONSOLE_H_
