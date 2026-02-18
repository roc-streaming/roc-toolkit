/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/backtrace.h
//! @brief Backtrace printing.

#ifndef ROC_CORE_BACKTRACE_H_
#define ROC_CORE_BACKTRACE_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Print backtrace to stderr.
//! @note
//!  This function is NOT signal-safe
//!  It CAN use heap and stdio.
void print_backtrace_full();

//! Print backtrace to stderr (emergency mode).
//! @note
//!  This function IS signal-safe.
//!  It can NOT use heap and stdio.
void print_backtrace_safe();

//! Demangle symbol name.
//! @note
//!  This function is NOT signal-safe.
//!  It CAN use heap and stdio.
//! @remarks
//!  @p demangled_buf and @p demangled_size specify the buffer for demangled name.
//!  When necessary, this function malloc()s or realloc()s @p demangled_buf and
//!  updates @p demangled_size accordingly. The buffer may be NULL. The buffer may
//!  be reused across several calls. The user should manually free() the buffer
//!  when it's not needed anymore.
//! @returns
//!  demangled symbol or NULL if the symbol can't be demangled.
const char*
demangle_symbol(const char* mangled, char*& demangled_buf, size_t& demangled_size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_BACKTRACE_H_
