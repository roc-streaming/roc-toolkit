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

#if defined(CLOCK_MONOTONIC)
uint64_t timestamp_ms() {
    timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        roc_panic("clock_gettime(CLOCK_MONOTONIC): %s", errno_to_str().c_str());
    }

    return uint64_t(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#else // !defined(CLOCK_MONOTONIC)
uint64_t timestamp_ms() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        roc_panic("gettimeofday: %s", errno_to_str().c_str());
    }

    return uint64_t(tv.tv_sec) * 1000 + uint64_t(tv.tv_usec) / 1000;
}
#endif // defined(CLOCK_MONOTONIC)

#if defined(CLOCK_MONOTONIC)
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
#else // !defined(CLOCK_MONOTONIC)
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
#endif // defined(CLOCK_MONOTONIC)

#if defined(CLOCK_MONOTONIC) && defined(TIMER_ABSTIME)
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
#else // !defined(CLOCK_MONOTONIC) || !defined(TIMER_ABSTIME)
void sleep_until_ms(uint64_t ms) {
    uint64_t now = timestamp_ms();
    if (ms > now) {
        sleep_for_ms(ms - now);
    }
}
#endif // defined(CLOCK_MONOTONIC) && defined(TIMER_ABSTIME)

} // namespace core
} // namespace roc
