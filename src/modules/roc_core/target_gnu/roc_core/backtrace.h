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
//! @note
//!  On Linux/GCC, this requires -rdynamic option, which is enabled in debug builds.
void print_backtrace();

} // namespace core
} // namespace roc

#endif // ROC_CORE_BACKTRACE_H_
