/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/demangle.h
//! @brief Symbol demangling.

#ifndef ROC_CORE_DEMANGLE_H_
#define ROC_CORE_DEMANGLE_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Demangle symbol name.
//!
//! This function is not signal-safe.
//!
//! @remarks
//!  @p demangled_buf and @p demangled_size specify the buffer for demangled name.
//!  When necessary, this function malloc()s or realloc()s @p demangled_buf and
//!  updates @p demangled_size accrodingly. The buffer may be NULL. The buffer may
//!  be resused across several calls. The user should manually free() the buffer
//!  when it's not needed anymore.
//!
//! @returns
//!  demangled symbol or NULL if the symbol can't be demangled.
const char* demangle(const char* mangled, char*& demangled_buf, size_t& demangled_size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_DEMANGLE_H_
