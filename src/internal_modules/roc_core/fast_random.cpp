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

// Atomic PRNG state.
uint32_t rng_state;

} // namespace

// A lock-free adaptation of splitmix32 by Tommy Ettinger:
// https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
uint32_t fast_random_32() {
    if (AtomicOps::load_relaxed(rng_state) == 0) {
        uint32_t expected_state = 0;
        uint32_t new_state = (uint32_t)core::timestamp(core::ClockUnix);
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

// Poor man's 64-bit PRNG derived from 32-bit PRNG.
// We don't want to implement 64-bit PRNG natively, because we need it lock-free,
// and 64-bit atomics are not available everywhere.
uint64_t fast_random_64() {
    const uint64_t hi = fast_random_32();
    const uint64_t lo = fast_random_32();

    return (hi << 32) | lo;
}

// Floats in [0; 1] have 24-bit precision, so 32-bit PRNG is enough.
float fast_random_float() {
    return (float)fast_random_32() / (float)UINT32_MAX;
}

// Bounded PRNG adaptation of "Bitmask with Rejection (Unbiased) — Apple's Method"
// algorithm: https://www.pcg-random.org/posts/bounded-rands.html
// This implementation is unbiased unlike simple modulo division, and allows
// 64-bit arithmetic without overflows unlike other approaches.
uint64_t fast_random_range(uint64_t from, uint64_t to) {
    roc_panic_if_msg(from > to, "fast random: invalid range: from=%llu to=%llu",
                     (unsigned long long)from, (unsigned long long)to);

    if (from == 0 && to == UINT64_MAX) {
        // Catch the only case when range overflows.
        return fast_random_64();
    }

    const uint64_t range = to - from + 1;

    // Generate a mask with 1's from bit 0 to the most significant bit in `range`.
    // At each step, we double the count of leading 1's:
    //  0001.......
    //  00011......
    //  0001111....
    // Thanks to @rnovatorov for the hint.
    uint64_t mask = range;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;
    mask |= mask >> 16;
    mask |= mask >> 32;

    uint64_t rnd;
    do {
        rnd = fast_random_64() & mask;
    } while (rnd >= range);

    const uint64_t ret = from + rnd;

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    return ret;
}

// Gaussian PRNG implementation is based on Box-Muller transform:
// https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
float fast_random_gaussian() {
    // Generate two uniform random numbers
    const float u1 = fast_random_float();
    const float u2 = fast_random_float();

    // Use Box-Muller transform to convert uniform random numbers to normal random numbers
    const float r = std::sqrt(-2.0f * std::log(u1));
    const float theta = 2.0f * float(M_PI) * u2;

    // Return one of the normal random numbers
    return r * std::cos(theta);
}

} // namespace core
} // namespace roc
