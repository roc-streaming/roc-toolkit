/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

#ifdef CLOCK_MONOTONIC

uint64_t timestamp_ms() {
    timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        roc_panic("clock_gettime(CLOCK_MONOTONIC): %s", errno_to_str().c_str());
    }

    return uint64_t(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
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

#else // !CLOCK_MONOTONIC

uint64_t timestamp_ms() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        roc_panic("gettimeofday: %s", errno_to_str().c_str());
    }

    return uint64_t(tv.tv_sec) * 1000 + uint64_t(tv.tv_usec) / 1000;
}

void sleep_for_ms(uint64_t ms) {
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;

    while (nanosleep(&ts, &ts) == -1) {
        if (errno != EINTR) {
            roc_panic("nanosleep: %s", errno_to_str().c_str());
        }
    }
}

#endif // CLOCK_MONOTONIC

#if defined(TIMER_ABSTIME) && defined(CLOCK_MONOTONIC)

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

#else // !(TIMER_ABSTIME && CLOCK_MONOTONIC)

void sleep_until_ms(uint64_t ms) {
    uint64_t now = timestamp_ms();
    if (ms > now) {
        sleep_for_ms(ms - now);
    }
}

#endif // TIMER_ABSTIME && CLOCK_MONOTONIC

} // namespace core
} // namespace roc
