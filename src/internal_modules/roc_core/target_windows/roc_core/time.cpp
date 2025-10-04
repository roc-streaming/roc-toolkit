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

#include <windows.h>

// Present when compiling with MinGW in Windows,
//  but not defined when cross-compiling from Linux (Debian 12)
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION (0x00000002)
#endif

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

nanoseconds_t timestamp(clock_t clock) {
    timespec ts;
    if (clock_gettime(map_clock(clock), &ts) == -1) {
        roc_panic("time: clock_gettime(): %s", errno_to_str().c_str());
    }

    return nanoseconds_t(ts.tv_sec) * 1000000000 + nanoseconds_t(ts.tv_nsec);
}

void sleep_for(clock_t clock, nanoseconds_t ns) {
    // TODO: handle fallback to non high-resolution timer? (waitable high resolution timer
    // supported only from W10 1803)
    HANDLE hTimer = CreateWaitableTimerEx(
        NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!hTimer) {
        roc_panic("time: CreateWaitableTimerEx(): %s", errno_to_str().c_str());
    }
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = ns / -100; // In 100 nanoseconds interval, negative = relative
                                  // value.
    // should we add -1 to round for "at least" xx ns?
    if (!SetWaitableTimer(hTimer, &dueTime, 0 /*once*/, NULL, NULL, FALSE)) {
        roc_panic("time: SetWaitableTimer(): %s", errno_to_str().c_str());
    }
    if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) {
        roc_panic("time: WaitForSingleObject(): %s", errno_to_str().c_str());
    }
    CloseHandle(hTimer);
}

void sleep_until(clock_t clock, nanoseconds_t ns) {
    nanoseconds_t now = timestamp(clock);
    if (ns > now) {
        sleep_for(clock, ns - now);
    }
}

std::tm nanoseconds_2_tm(nanoseconds_t timestamp) {
    const time_t sec = time_t(timestamp / Second);

    std::tm tm;
    errno_t err = localtime_s(&tm, &sec);
    if (err)
        roc_panic("time: localtime_s(): %s", errno_to_str(err).c_str());

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
