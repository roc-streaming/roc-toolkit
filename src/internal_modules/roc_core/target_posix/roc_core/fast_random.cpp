/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <pthread.h>
#include <stdlib.h>

#include "roc_core/atomic_ops.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/fast_random.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

namespace {

pthread_once_t once_control = PTHREAD_ONCE_INIT;

uint32_t state;

void init_state() {
    const nanoseconds_t seed_48 = timestamp(ClockMonotonic);
    state = (uint32_t)seed_48;
}

inline uint32_t splitmix32(uint32_t z) {
    z = z ^ (z >> 16);
    z *= 0x21F0AAAD;
    z = z ^ (z >> 15);
    z *= 0x735A2D97;
    z = z ^ (z >> 15);
    return z;
}

} // namespace

// The implementation is based on "Debiased Modulo (Once) — Java's Method" algorithm
// from https://www.pcg-random.org/posts/bounded-rands.html
//
// We use splitmix32 as a PRNG
// shifts and multiplcation value were taken from link below
// https://gist.github.com/tommyettinger/46a874533244883189143505d203312c?permalink_comment_id=4365431#gistcomment-4365431
//
// This implementation is not a cryptographically secure PRNG.
uint32_t fast_random(uint32_t from, uint32_t to) {
    roc_panic_if_not(from <= to);

    const uint64_t range = uint64_t(to) - from + 1;

    uint64_t z, r;

    if (int err = pthread_once(&once_control, init_state)) {
        roc_panic("fast random: pthread_once(): %s", errno_to_str(err).c_str());
    }

    do {
        z = AtomicOps::fetch_add_seq_cst(state, 0x9E3779B9);
        z = splitmix32(z);
        r = z % range;
    } while (z - r > (-range));

    const uint32_t ret = from + (uint32_t)r;

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    return ret;
}

} // namespace core
} // namespace roc
