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
#include "roc_core/string_builder.h"

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
//!  - @p src - source string in UTF-8
//!  - @p src_sz - source string size
//!  - @p mode - encoding mode
//!
//! @remarks
//!  The source string should NOT be null-terminated.
//!  The source string size should NOT include the terminating zero byte.
bool pct_encode(core::StringBuilder& dst, const char* src, size_t src_sz, PctMode mode);

//! Percent-decode an UTF-8 string.
//
//! @b Parameters
//!  - @p dst - destination buffer
//!  - @p src - source string in UTF-8
//!  - @p src_sz - source string size
//!
//! @remarks
//!  The source string should NOT be null-terminated.
//!  The source string size should NOT include the terminating zero byte.
bool pct_decode(core::StringBuilder& dst, const char* src, size_t src_sz);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_PCT_H_
