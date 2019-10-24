/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/string_utils.h
//! @brief String utility functions.

#ifndef ROC_CORE_STRING_UTILS_H_
#define ROC_CORE_STRING_UTILS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Copy bytes from [src_begin; src_end) to [dst; dst+dst_size).
//! @remarks
//!  - The source and destination shouldn't be NULL.
//!  - The source string should not be zero-terminated.
//!  - If destination is too small, the source string is truncated.
//!  - If destination is at least one byte, it will be zero-terminated in any case.
//! @returns
//!  true if the string was written completely or false if it was truncated.
//! @note
//!  The function is signal-safe.
bool copy_str(char* dst, size_t dst_size, const char* src_begin, const char* src_end);

//! Copy bytes from [src; src+strlen(src)) to [dst; dst+dst_size).
//! @remarks
//!  - The source and destination shouldn't be NULL.
//!  - The source string should be zero-terminated.
//!  - If destination is too small, the source string is truncated.
//!  - If destination is at least one byte, it will be zero-terminated in any case.
//! @returns
//!  true if the string was written completely or false if it was truncated.
//! @note
//!  The function is signal-safe.
bool copy_str(char* dst, size_t dst_size, const char* src);

//! Copy bytes from [src_begin; src_end) to [dst+strlen(dst); dst+dst_size).
//! @remarks
//!  - The source and destination shouldn't be NULL.
//!  - The source string should not be zero-terminated.
//!  - The destination string should be already zero-terminated.
//!  - If destination is too small, the source string is truncated.
//!  - The destination string will remain zero-terminated in any case.
//! @returns
//!  true if the string was written completely or false if it was truncated.
//! @note
//!  The function is signal-safe.
bool append_str(char* dst, size_t dst_size, const char* src_begin, const char* src_end);

//! Copy bytes from [src; src+strlen(src)) to [dst+strlen(dst); dst+dst_size).
//! @remarks
//!  - The source and destination shouldn't be NULL.
//!  - The source string should be zero-terminated.
//!  - The destination string should be already zero-terminated.
//!  - If destination is too small, the source string is truncated.
//!  - The destination string will remain zero-terminated in any case.
//! @returns
//!  true if the string was written completely or false if it was truncated.
//! @note
//!  The function is signal-safe.
bool append_str(char* dst, size_t dst_size, const char* src);

//! Format an integer to a string and append it to another string.
//! @remarks
//!  - Base should be in range [2; 16].
//!  - The destination shouldn't be NULL.
//!  - The destination string should be already zero-terminated.
//!  - If destination is too small, the formatted number is truncated.
//!  - The destination string will remain zero-terminated in any case.
//! @returns
//!  true if the number was written completely or false if it was truncated.
//! @note
//!  The function is signal-safe.
bool append_uint(char* dst, size_t dst_size, uint64_t number, unsigned int base);

} // namespace core
} // namespace roc

#endif // ROC_CORE_STRING_UTILS_H_
