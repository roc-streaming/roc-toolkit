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

//! Compute hash of 16-bit integer.
hashsum_t hashsum_int(int16_t);

//! Compute hash of 16-bit integer.
hashsum_t hashsum_int(uint16_t);

//! Compute hash of 32-bit integer.
hashsum_t hashsum_int(int32_t);

//! Compute hash of 32-bit integer.
hashsum_t hashsum_int(uint32_t);

//! Compute hash of 64-bit integer.
hashsum_t hashsum_int(int64_t);

//! Compute hash of 64-bit integer.
hashsum_t hashsum_int(uint64_t);

//! Compute hash of an integer.
//! This fallback is needed for the cases when the overloads above
//! do not cover all builtin types. E.g. if none of the overloads
//! above covers unsigned long or unsigned long long.
template <class T> hashsum_t hashsum_int(T t) {
    switch (sizeof(T)) {
    case 2:
        return hashsum_int((uint16_t)t);
    case 4:
        return hashsum_int((uint32_t)t);
    case 8:
        return hashsum_int((uint64_t)t);
    }
}

//! Compute hash of zero-terminated string.
hashsum_t hashsum_str(const char* str);

//! Compute hash of byte range.
hashsum_t hashsum_mem(const void* data, size_t size);

//! Incrementally compute hash of memory chunks.
//! On first invocation, @p hash should be zero.
void hashsum_add(hashsum_t& hash, const void* data, size_t size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHSUM_H_
