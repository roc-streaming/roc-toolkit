/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix_pc/roc_core/console.h
//! @brief Console.

#ifndef ROC_CORE_CONSOLE_H_
#define ROC_CORE_CONSOLE_H_

#include "roc_core/attributes.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"

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

    const bool colors_supported_;
    Mutex mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_CONSOLE_H_
