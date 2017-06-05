/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <time.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

uint64_t timestamp_ms() {
    timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        roc_panic("clock_gettime(CLOCK_MONOTONIC): %s", errno_to_str().c_str());
    }

    return uint64_t(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void sleep_until_ms(uint64_t ms) {
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;

    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == -1) {
        if (errno != EINTR) {
            roc_panic("clock_nanosleep(CLOCK_MONOTONIC): %s", errno_to_str().c_str());
        }
    }
}

void sleep_for_ms(uint64_t ms) {
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;

    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL) == -1) {
        if (errno != EINTR) {
            roc_panic("clock_nanosleep(CLOCK_MONOTONIC): %s", errno_to_str().c_str());
        }
    }
}

} // namespace core
} // namespace roc
