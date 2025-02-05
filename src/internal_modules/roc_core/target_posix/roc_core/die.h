/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/die.h
//! @brief Program termination.

#ifndef ROC_CORE_DIE_H_
#define ROC_CORE_DIE_H_

#include "roc_core/attributes.h"

namespace roc {
namespace core {

//! Terminate program.
//! @remarks
//!  Terminates immediately without calling destructors and exit handlers.
ROC_NORETURN void die_fast(int code);

//! Terminate program with error message and backtrace.
//! @remarks
//!  Prints error message, backtraces, and terminates program with error.
ROC_NORETURN void die_gracefully(const char* message, bool full_backtrace);

} // namespace core
} // namespace roc

#endif // ROC_CORE_DIE_H_
