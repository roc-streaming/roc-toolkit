/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/time.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace roc {
namespace core {

namespace {

#if defined(CLOCK_REALTIME)

clockid_t map_clock(clock_t clock) {
    if (clock == ClockMonotonic) {
#if defined(CLOCK_MONOTONIC)
        return CLOCK_MONOTONIC;
#endif
    }

    return CLOCK_REALTIME;
}

#endif

} // namespace

#if defined(CLOCK_REALTIME)

nanoseconds_t timestamp(clock_t clock) {
    timespec ts;
    if (clock_gettime(map_clock(clock), &ts) == -1) {
        roc_panic("time: clock_gettime(): %s", errno_to_str().c_str());
    }

    return nanoseconds_t(ts.tv_sec) * 1000000000 + nanoseconds_t(ts.tv_nsec);
}

#else

nanoseconds_t timestamp(clock_t clock) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        roc_panic("time: gettimeofday(): %s", errno_to_str().c_str());
    }

    return nanoseconds_t(tv.tv_sec) * 1000000000 + nanoseconds_t(tv.tv_usec) * 1000;
}

#endif

#if defined(CLOCK_REALTIME) && !defined(__APPLE__) && !defined(__MACH__)

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

#else

void sleep_for(clock_t clock, nanoseconds_t ns) {
    timespec ts;
    ts.tv_sec = time_t(ns / 1000000000);
    ts.tv_nsec = long(ns % 1000000000);

    while (nanosleep(&ts, &ts) == -1) {
        if (errno != EINTR) {
            roc_panic("time: nanosleep(): %s", errno_to_str().c_str());
        }
    }
}

#endif

#if defined(CLOCK_REALTIME) && defined(TIMER_ABSTIME)

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

#else

void sleep_until(clock_t clock, nanoseconds_t ns) {
    nanoseconds_t now = timestamp(clock);
    if (ns > now) {
        sleep_for(clock, ns - now);
    }
}

#endif

std::tm nanoseconds_2_tm(nanoseconds_t timestamp) {
    const time_t sec = time_t(timestamp / Second);

    std::tm tm;
    if (localtime_r(&sec, &tm) == NULL) {
        roc_panic("time: localtime_r(): %s", errno_to_str().c_str());
    }

    return tm;
}

nanoseconds_t tm_2_nanoseconds(std::tm tm) {
    const time_t sec = mktime(&tm);

    if (sec == (time_t)-1) {
        roc_panic("time: mktime(): %s", errno_to_str().c_str());
    }

    return nanoseconds_t(sec) * Second;
}

} // namespace core
} // namespace roc
