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
pthread_key_t rand_key;

pthread_once_t rand_key_once = PTHREAD_ONCE_INIT;

unsigned short rand_seed[3] = {};

} // namespace

void do_init();

// Thread initiliazer routine
void do_init() {
    pthread_key_create(&rand_key, NULL);
    if (int err = pthread_key_create(&rand_key, NULL)) {
        roc_panic("fast random: pthread_key_create(): %s", errno_to_str(err).c_str());
    }
    const nanoseconds_t seed_48 = timestamp();
    rand_seed[0] = (seed_48 & 0xffff);
    rand_seed[1] = ((seed_48 >> 16) & 0xffff);
    rand_seed[2] = ((seed_48 >> 32) & 0xffff);
    return;
}

// The implementation is based on "Debiased Modulo (Once) — Java's Method" algorithm
// from https://www.pcg-random.org/posts/bounded-rands.html
//
// We use nrand48(), which is thread-safe on most platforms. It is probably not fully
// thread-safe on glibc when used concurrently with lcong48(), but most likely the
// race is harmless. See https://www.evanjones.ca/random-thread-safe.html.
//
// This implementation is not a cryptographically secure PRNG.
uint32_t fast_random(uint32_t from, uint32_t to) {
    roc_panic_if_not(from <= to);

    const uint64_t range = uint64_t(to) - from + 1;

    uint64_t x, r;

    pthread_once(&rand_key_once, do_init);

    unsigned short* val = (unsigned short*)pthread_getspecific(rand_key);
    if (val == NULL) {
        val = rand_seed;
        pthread_setspecific(rand_key, &rand_seed);
    }

    do {
        x = (uint64_t)nrand48(val);
        r = x % range;
    } while (x - r > (-range));

    const uint32_t ret = from + (uint32_t)r;

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    return ret;
}

} // namespace core
} // namespace roc
