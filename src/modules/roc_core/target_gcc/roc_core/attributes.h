/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_gcc/roc_core/attributes.h
//! @brief GCC attributes.

#ifndef ROC_CORE_ATTRIBUTES_H_
#define ROC_CORE_ATTRIBUTES_H_

//! Function never throws.
#define ROC_ATTR_NOTHROW throw()

//! Function never returns.
#define ROC_ATTR_NORETURN __attribute__((noreturn))

//! Function or variable is never used but no warning should be generated.
#define ROC_ATTR_UNUSED __attribute__((unused))

//! Structure's fields are packed.
#define ROC_ATTR_PACKED __attribute__((packed))

//! Function gets printf-like arguments.
#define ROC_ATTR_PRINTF(n_fmt_arg, n_var_arg)                                            \
    __attribute__((format(printf, n_fmt_arg, n_var_arg)))

#ifdef __has_attribute
#if __has_attribute(no_sanitize)
#define ROC_ATTR_NO_SANITIZE_UB __attribute__((no_sanitize("undefined")))
#elif __has_attribute(no_sanitize_undefined)
#define ROC_ATTR_NO_SANITIZE_UB __attribute__((no_sanitize_undefined))
#endif //__has_attribute(no_sanitize)
#endif //__has_attribute

#ifndef ROC_ATTR_NO_SANITIZE_UB
//! Suppress undefined behavior sanitizer for a particular function.
#define ROC_ATTR_NO_SANITIZE_UB
#endif

//! Explicitly specify a default visibility for a specific symbol.
#define ROC_ATTR_EXPORT __attribute__((visibility("default")))

#endif // ROC_CORE_ATTRIBUTES_H_
