/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/attributes.h
//! @brief Compiler attributes.

#ifndef ROC_CORE_ATTRIBUTES_H_
#define ROC_CORE_ATTRIBUTES_H_

#include <hedley.h>

//! Set "default" visibility for symbol.
//! Without this, "hidden" visibility is used for symbols.
#define ROC_EXPORT HEDLEY_PUBLIC

#if HEDLEY_HAS_ATTRIBUTE(aligned)
//! Align structure field.
#define ROC_ALIGNED(x) __attribute__((aligned(x)))
#endif

#if HEDLEY_HAS_ATTRIBUTE(packed) || HEDLEY_GCC_VERSION
//! Pack structure fields.
//! Place these before class or struct keyword.
#define ROC_PACKED_BEGIN
//! Pack structure fields.
//! Place these between '}' and ';'.
#define ROC_PACKED_END __attribute__((packed))
#endif

//! Hint for compiler that function takes printf-like arguments.
//! Compiler will emit warnings on mis-use.
#define ROC_PRINTF(fmt_pos, args_pos) HEDLEY_PRINTF_FORMAT(fmt_pos, args_pos)

#if defined(HEDLEY_GNUC_VERSION)
//! Hint for compiler that function never returns.
#define ROC_NORETURN __attribute__((__noreturn__))
#else
//! Hint for compiler that function never returns.
#define ROC_NORETURN HEDLEY_NO_RETURN
#endif

#ifdef HEDLEY_GCC_VERSION
//! Emit warning if function result is not checked.
#define ROC_NODISCARD // GCC is too aggressive with this attribute.
#else
//! Emit warning if function result is not checked.
#define ROC_NODISCARD HEDLEY_WARN_UNUSED_RESULT
#endif

#if HEDLEY_HAS_ATTRIBUTE(unused)
//! Don't emit warning if function or variable is never used.
#define ROC_NOUNUSED __attribute__((unused))
#else
//! Don't emit warning if function or variable is never used.
#define ROC_NOUNUSED
#endif

#if HEDLEY_HAS_ATTRIBUTE(no_sanitize)
//! Suppress sanitizers for a particular function.
#define ROC_NOSANITIZE __attribute__((no_sanitize("undefined")))
#elif HEDLEY_HAS_ATTRIBUTE(no_sanitize_undefined)
//! Suppress sanitizers for a particular function.
#define ROC_NOSANITIZE __attribute__((no_sanitize_undefined))
#else
//! Suppress sanitizers for a particular function.
#define ROC_NOSANITIZE
#endif

#endif // ROC_CORE_ATTRIBUTES_H_
