/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/fast_random.h
//! @brief Helpers to work with random numbers.

#ifndef ROC_CORE_FAST_RANDOM_H_
#define ROC_CORE_FAST_RANDOM_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Get a random integer from a non cryptographically secure, but fast PRNG.
//! Thread-safe.
//! @returns random value in range [from; to].
uint32_t fast_random(uint32_t from, uint32_t to);

} // namespace core
} // namespace roc

#endif // ROC_CORE_FAST_RANDOM_H_
