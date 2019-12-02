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
#include "roc_core/mutex.h"
#include "roc_core/panic.h"
#include "roc_core/random.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

namespace {

pthread_once_t rand_once = PTHREAD_ONCE_INIT;

unsigned short rand_seed[3] = {};

void random_init() {
    nanoseconds_t seed_48 = timestamp();
    rand_seed[0] = (seed_48 & 0xffff);
    rand_seed[1] = ((seed_48 >> 16) & 0xffff);
    rand_seed[2] = ((seed_48 >> 32) & 0xffff);
}

} // namespace

// Based on arc4random_uniform() from OpenBSD.
uint32_t random(uint32_t from, uint32_t to) {
    if (int err = pthread_once(&rand_once, random_init)) {
        roc_panic("pthread_once: %s", errno_to_str(err).c_str());
    }

    roc_panic_if_not(from <= to);

    uint64_t upper = uint64_t(to) - from + 1;
    uint64_t min = -upper % upper;
    uint64_t val = 0;

    for (;;) {
        if ((val = (uint64_t)nrand48(rand_seed)) >= min) {
            break;
        }
    }

    uint32_t ret = from + uint32_t(val % upper);

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    return ret;
}

} // namespace core
} // namespace roc
