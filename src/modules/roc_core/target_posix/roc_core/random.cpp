/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdlib.h>

#include "roc_core/panic.h"
#include "roc_core/random.h"
#include "roc_core/time.h"
#include "roc_core/mutex.h"

namespace roc {
namespace core {

namespace {

Mutex rand_mutex;

bool rand_init_done = false;

unsigned short rand_seed[3] = {};

} // namespace

void random_init(uint64_t seed_48) {
    rand_seed[0] = (seed_48 & 0xffff);
    rand_seed[1] = ((seed_48 >> 16) & 0xffff);
    rand_seed[2] = ((seed_48 >> 32) & 0xffff);
    rand_init_done = true;
}

// Insecure, but (hopefully?) uniform and thread-safe implementation.
// See arc4random_uniform() from OpenBSD.
unsigned random(unsigned from, unsigned to) {
    Mutex::Lock lock(rand_mutex);

    if (!rand_init_done) {
        random_init(timestamp_ms());
    }

    roc_panic_if_not(from <= to);

    uint32_t upper = uint32_t(to - from + 1);
    uint32_t min = -upper % upper;
    uint32_t val = 0;

    for (;;) {
        if ((val = (uint32_t)nrand48(rand_seed)) >= min) {
            break;
        }
    }

    unsigned ret = from + (unsigned)(val % upper);

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    return ret;
}

unsigned random(unsigned upper) {
    roc_panic_if_not(upper > 0);

    return random(0, upper - 1);
}

} // namespace core
} // namespace roc
