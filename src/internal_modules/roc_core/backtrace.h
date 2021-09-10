/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/backtrace.h
//! @brief Backtrace printing.

#ifndef ROC_CORE_BACKTRACE_H_
#define ROC_CORE_BACKTRACE_H_

namespace roc {
namespace core {

//! Print backtrace to stderr.
//! @remarks
//!  This function is not signal-safe.
//!  It can use heap and stdio.
void print_backtrace();

//! Print backtrace to stderr (emergency mode).
//! @remarks
//!  This function is signal-safe.
//!  It can't use heap and stdio.
void print_emergency_backtrace();

//! Print message to stderr (emergency mode).
//! @remarks
//!  This function is signal-safe.
//!  It can't use heap and stdio.
void print_emergency_message(const char* str);

} // namespace core
} // namespace roc

#endif // ROC_CORE_BACKTRACE_H_
