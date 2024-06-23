/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/fast_random.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

namespace {

uint32_t rng_state;

} // namespace

// PRNG implementation is a lock-free adaptation of splitmix32 by Tommy Ettinger:
// https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
uint32_t fast_random() {
    if (AtomicOps::load_relaxed(rng_state) == 0) {
        uint32_t expected_state = 0;
        uint32_t new_state = (uint32_t)core::timestamp(core::ClockMonotonic);
        AtomicOps::compare_exchange_seq_cst(rng_state, expected_state, new_state);
    }

    uint32_t z;

    z = AtomicOps::fetch_add_seq_cst(rng_state, 0x9E3779B9);
    z = z ^ (z >> 16);
    z *= 0x21F0AAAD;
    z = z ^ (z >> 15);
    z *= 0x735A2D97;
    z = z ^ (z >> 15);
    return z;
}

// Bounded PRNG implementation is based on "Debiased Modulo (Once) — Java's Method"
// algorithm: https://www.pcg-random.org/posts/bounded-rands.html
uint32_t fast_random_range(uint32_t from, uint32_t to) {
    roc_panic_if_not(from <= to);

    const uint64_t range = uint64_t(to) - from + 1;

    uint64_t z, r;

    do {
        z = fast_random();
        r = z % range;
    } while (z - r > (-range));

    const uint32_t ret = from + (uint32_t)r;

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    return ret;
}

// Gaussian PRNG implementation is based on Box-Muller transform:
// https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
double fast_random_gaussian() {
    // Generate two uniform random numbers
    double u1 = (double)fast_random() / (double)UINT32_MAX;
    double u2 = (double)fast_random() / (double)UINT32_MAX;

    // Use Box-Muller transform to convert uniform random numbers to normal random numbers
    double r = std::sqrt(-2.0 * std::log(u1));
    double theta = 2.0 * M_PI * u2;

    // Return one of the normal random numbers
    return r * std::cos(theta);
}

} // namespace core
} // namespace roc
