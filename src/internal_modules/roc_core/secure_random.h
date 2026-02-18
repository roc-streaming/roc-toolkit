/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/secure_random.h
//! @brief Helpers to generate cryptographically secure random numbers.

#ifndef ROC_CORE_SECURE_RANDOM_H_
#define ROC_CORE_SECURE_RANDOM_H_

#include "roc_core/attributes.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Fill buffer @p buf with @p bufsz random bites.
//! Thread-safe. Uniformly distributed.
//!
//! @returns @p true in case of success, @p false in case of failure.
//!
//! @warning On some platforms CSPRNG is not available. In this case a non-secure version
//! may be used instead (e.g. @p fast_random*()).
ROC_NODISCARD
bool secure_random(void* buf, size_t bufsz);

//! Get random 32-bit integer in range [@p from; @p to] and put it to @p dest.
//! Thread-safe. Uniformly distributed.
//!
//! @returns @p true in case of success, @p false in case of failure.
//!
//! @warning On some platforms CSPRNG is not available. In this case a non-secure version
//! may be used instead (e.g. @p fast_random*()).
ROC_NODISCARD
bool secure_random_range_32(uint32_t from, uint32_t to, uint32_t& dest);

//! Get random 64-bit integer in range [@p from; @p to] and put it to @p dest.
//! Thread-safe. Uniformly distributed.
//!
//! @returns @p true in case of success, @p false in case of failure.
//!
//! @warning On some platforms CSPRNG is not available. In this case a non-secure version
//! may be used instead (e.g. @p fast_random*()).
ROC_NODISCARD
bool secure_random_range_64(uint64_t from, uint64_t to, uint64_t& dest);

} // namespace core
} // namespace roc

#endif // ROC_CORE_SECURE_RANDOM_H_
