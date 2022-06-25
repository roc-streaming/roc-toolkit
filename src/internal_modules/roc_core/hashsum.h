/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/hashsum.h
//! @brief Hash sum.

#ifndef ROC_CORE_HASHSUM_H_
#define ROC_CORE_HASHSUM_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Hash type.
typedef size_t hashsum_t;

//! Compute hash for 16-bit integer.
hashsum_t hashsum_int(int16_t);

//! Compute hash for 16-bit integer.
hashsum_t hashsum_int(uint16_t);

//! Compute hash for 32-bit integer.
hashsum_t hashsum_int(int32_t);

//! Compute hash for 32-bit integer.
hashsum_t hashsum_int(uint32_t);

//! Compute hash for 64-bit integer.
hashsum_t hashsum_int(int64_t);

//! Compute hash for 64-bit integer.
hashsum_t hashsum_int(uint64_t);

//! Compute hash for zero-terminated string.
hashsum_t hashsum_str(const char* str);

//! Compute hash for byte range.
hashsum_t hashsum_mem(const void* data, size_t size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHSUM_H_
