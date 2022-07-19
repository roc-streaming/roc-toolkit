/*
 * Copyright (c) 2015 Roc Streaming authors
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

#if defined(CLOCK_REALTIME)
#define HAS_CLOCKS
#endif

namespace roc {
namespace core {

namespace {

#if defined(HAS_CLOCKS)

clockid_t map_clock(clock_t clock) {
#if defined(CLOCK_MONOTONIC)
    if (clock == ClockMonotonic) {
        return CLOCK_MONOTONIC;
    }
#else
    (void)clock;
#endif

    return CLOCK_REALTIME;
}

#endif // defined(HAS_CLOCKS)

} // namespace

#if defined(HAS_CLOCKS)

nanoseconds_t timestamp(clock_t clock) {
    timespec ts;
    if (clock_gettime(map_clock(clock), &ts) == -1) {
        roc_panic("time: clock_gettime(): %s", errno_to_str().c_str());
    }

    return nanoseconds_t(ts.tv_sec) * 1000000000 + nanoseconds_t(ts.tv_nsec);
}

#else // !defined(HAS_CLOCKS)

nanoseconds_t timestamp(clock_t) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        roc_panic("time: gettimeofday(): %s", errno_to_str().c_str());
    }

    return nanoseconds_t(tv.tv_sec) * 1000000000 + nanoseconds_t(tv.tv_usec) * 1000;
}

#endif // defined(HAS_CLOCKS)

#if defined(HAS_CLOCKS)

void sleep_for(clock_t clock, nanoseconds_t ns) {
    timespec ts;
    ts.tv_sec = time_t(ns / 1000000000);
    ts.tv_nsec = long(ns % 1000000000);

    int err;
    while ((err = clock_nanosleep(map_clock(clock), 0, &ts, &ts))) {
        if (err != EINTR) {
            roc_panic("time: clock_nanosleep(): %s", errno_to_str(err).c_str());
        }
    }
}

#else // !defined(HAS_CLOCKS)

void sleep_for(clock_t, nanoseconds_t ns) {
    timespec ts;
    ts.tv_sec = time_t(ns / 1000000000);
    ts.tv_nsec = long(ns % 1000000000);

    while (nanosleep(&ts, &ts) == -1) {
        if (errno != EINTR) {
            roc_panic("time: nanosleep(): %s", errno_to_str().c_str());
        }
    }
}

#endif // defined(HAS_CLOCKS)

#if defined(HAS_CLOCKS) && defined(TIMER_ABSTIME)

void sleep_until(clock_t clock, nanoseconds_t ns) {
    timespec ts;
    ts.tv_sec = time_t(ns / 1000000000);
    ts.tv_nsec = long(ns % 1000000000);

    int err;
    while ((err = clock_nanosleep(map_clock(clock), TIMER_ABSTIME, &ts, NULL))) {
        if (err != EINTR) {
            roc_panic("time: clock_nanosleep(): %s", errno_to_str(err).c_str());
        }
    }
}

#else // !defined(HAS_CLOCKS) || !defined(TIMER_ABSTIME)

void sleep_until(clock_t clock, nanoseconds_t ns) {
    nanoseconds_t now = timestamp(clock);
    if (ns > now) {
        sleep_for(ns - now);
    }
}

#endif // defined(HAS_CLOCKS) && defined(TIMER_ABSTIME)

} // namespace core
} // namespace roc
