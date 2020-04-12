/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libuv/roc_core/secure_random.h
//! @brief Helpers to work with CSPRNG.

#ifndef ROC_CORE_SECURE_RANDOM_H_
#define ROC_CORE_SECURE_RANDOM_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Get a random integer from a cryptographically secure pseudorandom number
//! generator(CSPRNG).
//! @remarks
//!  Writes a random value in [from; to] to @p result.
//!  May block indefinitely.
//!  Falls back to a regular PRNG when using libuv version before 1.33.0.
//! @returns true on success or false if accessing CSPRNG failed.
bool secure_random(uint32_t from, uint32_t to, uint32_t& result);

} // namespace core
} // namespace roc

#endif // ROC_CORE_SECURE_RANDOM_H_
