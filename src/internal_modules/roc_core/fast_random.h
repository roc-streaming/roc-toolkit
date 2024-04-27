/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/fast_random.h
//! @brief Helpers to work with random numbers.

#ifndef ROC_CORE_FAST_RANDOM_H_
#define ROC_CORE_FAST_RANDOM_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Get a random integer from a non cryptographically secure, but fast PRNG.
//! Thread-safe.
//! @returns random value between 0 and UINT32_MAX.
uint32_t fast_random();

//! Get a random integer from a non cryptographically secure, but fast PRNG.
//! Thread-safe.
//! @returns random value in inclusive range [from; to].
uint32_t fast_random_range(uint32_t from, uint32_t to);

//! Get a random double from a non cryptographically secure, but fast PRNG.
//! Thread-safe.
//! @returns normally distibure random value with 1 variance.
double fast_random_gaussian();

} // namespace core
} // namespace roc

#endif // ROC_CORE_FAST_RANDOM_H_
