/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/hash.h
//! @brief Hash.

#ifndef ROC_CORE_HASH_H_
#define ROC_CORE_HASH_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Hash type.
typedef size_t hash_t;

//! Compute hash for 32-bit integer.
hash_t hash_int(int32_t);

//! Compute hash for 32-bit integer.
hash_t hash_int(uint32_t);

//! Compute hash for 64-bit integer.
hash_t hash_int(int64_t);

//! Compute hash for 64-bit integer.
hash_t hash_int(uint64_t);

//! Compute hash for zero-terminated string.
hash_t hash_str(const char* str);

//! Compute hash for byte range.
hash_t hash_mem(const void* data, size_t size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASH_H_
