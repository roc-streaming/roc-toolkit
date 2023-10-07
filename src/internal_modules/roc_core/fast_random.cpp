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

uint32_t state;

} // namespace

// PRNG implementation is a lock-free adaptation of splitmix32 by Tommy Ettinger:
// https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
//
// This implementation is not a cryptographically secure PRNG.
uint32_t fast_random() {
    if (AtomicOps::load_relaxed(state) == 0) {
        uint32_t expected_state = 0;
        uint32_t new_state = (uint32_t)core::timestamp(core::ClockMonotonic);
        AtomicOps::compare_exchange_seq_cst(state, expected_state, new_state);
    }

    uint32_t z;

    z = AtomicOps::fetch_add_seq_cst(state, 0x9E3779B9);
    z = z ^ (z >> 16);
    z *= 0x21F0AAAD;
    z = z ^ (z >> 15);
    z *= 0x735A2D97;
    z = z ^ (z >> 15);
    return z;
}

// Bounded PRNG implementation is based on "Debiased Modulo (Once) — Java's Method"
// algorithm: https://www.pcg-random.org/posts/bounded-rands.html
//
// This implementation is not a cryptographically secure PRNG.
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

} // namespace core
} // namespace roc
