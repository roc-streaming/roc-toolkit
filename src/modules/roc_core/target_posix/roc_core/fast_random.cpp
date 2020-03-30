/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <pthread.h>
#include <stdlib.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/fast_random.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

namespace {

pthread_mutex_t rand_mutex = PTHREAD_MUTEX_INITIALIZER;

bool rand_init = false;

unsigned short rand_seed[3] = {};

} // namespace

// The implementation is based on arc4random_uniform() from OpenBSD to provide
// a uniform distribution inside the requested range.
//
// We use nrand48(), which is thread-safe on most platforms. It is probably not fully
// thread-safe on glibc when used concurrently with lcong48(), but most likely the
// race is harmless. See https://www.evanjones.ca/random-thread-safe.html.
//
// This implementation is not a cryptographically strong PRNG.
uint32_t fast_random(uint32_t from, uint32_t to) {
    roc_panic_if_not(from <= to);

    const uint64_t upper = uint64_t(to) - from + 1;
    const uint64_t min = -upper % upper;

    if (int err = pthread_mutex_lock(&rand_mutex)) {
        roc_panic("pthread_mutex_lock: %s", errno_to_str(err).c_str());
    }

    if (!rand_init) {
        rand_init = true;
        const nanoseconds_t seed_48 = timestamp();
        rand_seed[0] = (seed_48 & 0xffff);
        rand_seed[1] = ((seed_48 >> 16) & 0xffff);
        rand_seed[2] = ((seed_48 >> 32) & 0xffff);
    }

    uint64_t val = 0;
    for (;;) {
        if ((val = (uint64_t)nrand48(rand_seed)) >= min) {
            break;
        }
    }

    if (int err = pthread_mutex_unlock(&rand_mutex)) {
        roc_panic("pthread_mutex_unlock: %s", errno_to_str(err).c_str());
    }

    const uint32_t ret = from + uint32_t(val % upper);

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    return ret;
}

} // namespace core
} // namespace roc
