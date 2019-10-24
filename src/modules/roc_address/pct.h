/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/pct.h
//! @brief Percent-encoding and -decoding.

#ifndef ROC_ADDRESS_PCT_H_
#define ROC_ADDRESS_PCT_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace address {

//! Percent-encoding mode.
enum PctMode {
    //! Percent-encode all symbols that are not unreserved.
    PctNonUnreserved,

    //! Percent-encode all symbols that are not allowed in host.
    PctNonHost,

    //! Percent-encode all symbols that are not allowed in path.
    PctNonPath
};

//! Percent-encode an UTF-8 string.
//
//! @b Parameters
//!  - @p dst - destination buffer
//!  - @p dst_sz - destination buffer size
//!  - @p src - source string in UTF-8
//!  - @p src_sz - source string size
//!  - @p mode - encoding mode
//!
//! @returns
//!  number of characters written to destination buffer, excluding the terminating
//!  zero byte, or -1 if the buffer is too small or the source string is invalid.
//!
//! @remarks
//!  The source string should NOT be null-terminated.
//!  The source string size should NOT include the terminating zero byte.
//!  The destination buffer size SHOULD include the terminating zero byte.
//!  If the function succeeded, the resulting string is ALWAYS null-terminated,
//!  but the returned size EXCLUDES the terminating zero byte.
ssize_t
pct_encode(char* dst, size_t dst_sz, const char* src, size_t src_sz, PctMode mode);

//! Percent-decode an UTF-8 string.
//
//! @b Parameters
//!  - @p dst - destination buffer
//!  - @p dst_sz - destination buffer size
//!  - @p src - source string in UTF-8
//!  - @p src_sz - source string size
//!
//! @returns
//!  number of characters written to destination buffer, excluding the terminating
//!  zero byte, or -1 if the buffer is too small or the source string is invalid.
//!
//! @remarks
//!  The source string should NOT be null-terminated.
//!  The source string size should NOT include the terminating zero byte.
//!  The destination buffer size SHOULD include the terminating zero byte.
//!  If the function succeeded, the resulting string is ALWAYS null-terminated,
//!  but the returned size EXCLUDES the terminating zero byte.
ssize_t pct_decode(char* dst, size_t dst_sz, const char* src, size_t src_sz);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_PCT_H_
