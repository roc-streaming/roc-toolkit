/*
 * Copyright (c) 2015 Roc authors
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
nanoseconds_t timestamp() {
    timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        roc_panic("time: clock_gettime(CLOCK_MONOTONIC): %s", errno_to_str().c_str());
    }
    return nanoseconds_t(ts.tv_sec) * 1000000000 + nanoseconds_t(ts.tv_nsec);
}
#else  // !defined(CLOCK_MONOTONIC)
nanoseconds_t timestamp() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        roc_panic("time: gettimeofday(): %s", errno_to_str().c_str());
    }
    return nanoseconds_t(tv.tv_sec) * 1000000000 + nanoseconds_t(tv.tv_usec) * 1000;
}
#endif // defined(CLOCK_MONOTONIC)

#if defined(CLOCK_MONOTONIC)
void sleep_for(nanoseconds_t ns) {
    timespec ts;
    ts.tv_sec = time_t(ns / 1000000000);
    ts.tv_nsec = long(ns % 1000000000);
    int err;
    while ((err = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts))) {
        if (err != EINTR) {
            roc_panic("time: clock_nanosleep(CLOCK_MONOTONIC): %s",
                      errno_to_str(err).c_str());
        }
    }
}
#else  // !defined(CLOCK_MONOTONIC)
void sleep_for(nanoseconds_t ns) {
    timespec ts;
    ts.tv_sec = time_t(ns / 1000000000);
    ts.tv_nsec = long(ns % 1000000000);
    while (nanosleep(&ts, &ts) == -1) {
        if (errno != EINTR) {
            roc_panic("time: nanosleep(): %s", errno_to_str().c_str());
        }
    }
}
#endif // defined(CLOCK_MONOTONIC)

#if defined(CLOCK_MONOTONIC) && defined(TIMER_ABSTIME)
void sleep_until(nanoseconds_t ns) {
    timespec ts;
    ts.tv_sec = time_t(ns / 1000000000);
    ts.tv_nsec = long(ns % 1000000000);
    int err;
    while ((err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL))) {
        if (err != EINTR) {
            roc_panic("time: clock_nanosleep(CLOCK_MONOTONIC): %s",
                      errno_to_str(err).c_str());
        }
    }
}
#else  // !defined(CLOCK_MONOTONIC) || !defined(TIMER_ABSTIME)
void sleep_until(nanoseconds_t ns) {
    nanoseconds_t now = timestamp_ns();
    if (ns > now) {
        sleep_for(ns - now);
    }
}
#endif // defined(CLOCK_MONOTONIC) && defined(TIMER_ABSTIME)

} // namespace core
} // namespace roc
