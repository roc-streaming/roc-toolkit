/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/panic.h
//! @brief Panic.

#ifndef ROC_CORE_PANIC_H_
#define ROC_CORE_PANIC_H_

#include "roc_core/attributes.h"
#include "roc_core/macro_helpers.h"

#ifndef ROC_MODULE
#error "ROC_MODULE not defined"
#endif

//! Panic if condition is true.
//! @note
//!  This is a shorthand; roc_panic() with a meaningful error message is
//!  preferred way to panic.
#define roc_panic_if(x)                                                                  \
    do {                                                                                 \
        if ((x)) {                                                                       \
            roc_panic("%s", #x);                                                         \
        }                                                                                \
    } while (0)

//! Panic if condition is false.
//! @note
//!  This is a shorthand; roc_panic() with a meaningful error message is
//!  preferred way to panic.
#define roc_panic_if_not(x) roc_panic_if(!(x))

//! Panic if condition is true, printing custom message.
#define roc_panic_if_msg(x, ...)                                                         \
    do {                                                                                 \
        if ((x)) {                                                                       \
            roc_panic(__VA_ARGS__);                                                      \
        }                                                                                \
    } while (0)

//! Print error message and terminate program gracefully.
//! @remarks
//!  Never returns and never throws.
#define roc_panic(...)                                                                   \
    ::roc::core::panic(ROC_STRINGIZE(ROC_MODULE), __FILE__, __LINE__, __VA_ARGS__)

namespace roc {
namespace core {

//! Print error message and terminate program gracefully.
ROC_NORETURN ROC_PRINTF(4, 5) void panic(
    const char* module_name, const char* file, int line, const char* format, ...);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PANIC_H_
