/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_gnu/roc_core/backtrace.h
//! @brief Backtrace printing.

#ifndef ROC_CORE_BACKTRACE_H_
#define ROC_CORE_BACKTRACE_H_

namespace roc {
namespace core {

//! Print backtrace to stderr.
//! @remarks
//!  This function tries to performs symbol demangling, which uses signal-unsafe
//!  functions and works only with -rdynamic option (enabled in debug builds).
void print_backtrace();

//! Print backtrace to stderr (emergency mode).
//! @remarks
//!  This function does not use signal-unsafe functions and doesn't perform
//!  symbol demangling for this reason.
void print_emergency_backtrace();

//! Print string to stderr (emergency mode).
//! @remarks
//!  This function does not use signal-unsafe functions.
void print_emergency_string(const char* str);

} // namespace core
} // namespace roc

#endif // ROC_CORE_BACKTRACE_H_
