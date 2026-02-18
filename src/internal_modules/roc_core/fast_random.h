/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/fast_random.h
//! @brief Fast lock-free PRNG.

#ifndef ROC_CORE_FAST_RANDOM_H_
#define ROC_CORE_FAST_RANDOM_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Get random 32-bit integer in range [0; UINT32_MAX].
//! Thread-safe and lock-free.
//! Uniformly distributed.
//! Not cryptographically secure.
uint32_t fast_random_32();

//! Get random 64-bit integer in range [0; UINT64_MAX].
//! Thread-safe and lock-free.
//! Uniformly distributed.
//! Not cryptographically secure.
uint64_t fast_random_64();

//! Get random 32-bit float in range [0; 1].
//! Thread-safe and lock-free.
//! Uniformly distributed.
//! Not cryptographically secure.
float fast_random_float();

//! Get random 64-bit integer in range [from; to].
//! Thread-safe and lock-free.
//! Uniformly distributed.
//! Not cryptographically secure.
uint64_t fast_random_range(uint64_t from, uint64_t to);

//! Get random 32-bit float with standard normal distribution.
//! Thread-safe and lock-free.
//! Gaussian distribution N(0,1).
//! Not cryptographically secure.
float fast_random_gaussian();

} // namespace core
} // namespace roc

#endif // ROC_CORE_FAST_RANDOM_H_
