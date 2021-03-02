/*
 * Copyright (c) 2015 Roc authors
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

//! Explicitly specify a default visibility for a specific symbol.
#define ROC_ATTR_EXPORT HEDLEY_PUBLIC

//! Function never returns.
#define ROC_ATTR_NORETURN HEDLEY_NO_RETURN

//! Function gets printf-like arguments.
#define ROC_ATTR_PRINTF(fmt_pos, args_pos) HEDLEY_PRINTF_FORMAT(fmt_pos, args_pos)

#if HEDLEY_HAS_ATTRIBUTE(unused)
//! Function or variable is never used but no warning should be generated.
#define ROC_ATTR_UNUSED __attribute__((unused))
#else
//! Function or variable is never used but no warning should be generated.
#define ROC_ATTR_UNUSED
#endif

#if HEDLEY_HAS_ATTRIBUTE(packed)
//! Pack structure fields.
//! Place these before class or struct keyword.
#define ROC_ATTR_PACKED_BEGIN
//! Pack structure fields.
//! Place these between '}' and ';'.
#define ROC_ATTR_PACKED_END __attribute__((packed))
#else
//! Pack structure fields.
//! Place these before class or struct keyword.
#define ROC_ATTR_PACKED_BEGIN
//! Pack structure fields.
//! Place these between '}' and ';'.
#define ROC_ATTR_PACKED_END
#endif

#if HEDLEY_HAS_ATTRIBUTE(no_sanitize)
//! Suppress undefined behavior sanitizer for a particular function.
#define ROC_ATTR_NO_SANITIZE_UB __attribute__((no_sanitize("undefined")))
#elif HEDLEY_HAS_ATTRIBUTE(no_sanitize_undefined)
//! Suppress undefined behavior sanitizer for a particular function.
#define ROC_ATTR_NO_SANITIZE_UB __attribute__((no_sanitize_undefined))
#else
//! Suppress undefined behavior sanitizer for a particular function.
#define ROC_ATTR_NO_SANITIZE_UB
#endif

#endif // ROC_CORE_ATTRIBUTES_H_
