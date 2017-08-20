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

#if defined(CLOCK_MONOTONIC) && defined(TIMER_ABSTIME)
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
#elif defined(__APPLE__) && defined(__MACH__)

#include <mach/clock.h>
#include <mach/clock_types.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

/* mach_absolute_time() returns a Mach Time unit - clock ticks. The
 * length of a tick is a CPU dependent. On most Intel CPUs it probably
 * will be 1 nanoseconds per tick, but let's not rely on this. Mach
 * provides a conversation factor that can be used to convert abstract
 * mach time units to nanoseconds.
 */
static double steady_factor() {
    static double sf = 0;
    static uint64_t tm_start = 0;

    if (!tm_start) {
        mach_timebase_info_data_t info;
        kern_return_t ret = mach_timebase_info(&info);
        if (ret != KERN_SUCCESS) {
            roc_panic("mach_timebase_info: %d", ret);
        }
        sf = (double) info.numer / info.denom;
        tm_start = 1;
    }

    return sf;
}

/* As Apple mentioned: "The mach_timespec_t API is deprecated in OS X. The
 * newer and preferred API is based on timer objects that in turn use
 * AbsoluteTime as the basic data type".
 *
 * https://developer.apple.com/library/content/documentation/Darwin/Conceptual/KernelProgramming/Mach/Mach.html#//apple_ref/doc/uid/TP30000905-CH209-TPXREF111
 */
uint64_t timestamp_ms() {
    uint64_t now = uint64_t(mach_absolute_time() * steady_factor());
    return now / 1000000;
}

void sleep_until_ms(uint64_t ms) {
    mach_timespec_t ts;
    ts.tv_sec = (unsigned int)ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;

    kern_return_t ret = KERN_SUCCESS;
    for (;;) {
        // We are interested in using SYSTEM_CLOCK (aka CLOCK_MONOTONIC in
        // other world), digging into XNU source code we can find that if we
        // set a name for a clock port to MACH_PORT_NULL, a kernel will use
        // SYSTEM_CLOCK.
        //
        // https://opensource.apple.com/source/xnu/xnu-2422.1.72/osfmk/kern/clock_oldops.c
        // We are interested in a @clock_sleep_trap, because it's used by
        // @clock_sleep under the hood.
        ret = clock_sleep(MACH_PORT_NULL, TIME_ABSOLUTE, ts, NULL);
        if (ret == KERN_SUCCESS) {
            break;
        }

        if (ret != EINTR) {
            roc_panic("clock_sleep(TIME_ABSOLUTE): %d", ret);
        }
    }
}

void sleep_for_ms(uint64_t ms) {
    mach_timespec_t ts;
    ts.tv_sec = (unsigned int)ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;

    kern_return_t ret = KERN_SUCCESS;
    for (;;) {
        ret = clock_sleep(MACH_PORT_NULL, TIME_RELATIVE, ts, NULL);
        if (ret == KERN_SUCCESS) {
            break;
        }

        if (ret != EINTR) {
            roc_panic("clock_sleep(TIME_RELATIVE): %d", ret);
        }
    }
}
#else
uint64_t timestamp_ms() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        roc_panic("gettimeofday: %s", errno_to_str().c_str());
    }

    return uint64_t(tv.tv_sec) * 1000 + uint64_t(tv.tv_usec) / 1000;
}

void sleep_until_ms(uint64_t ms) {
    uint64_t now = timestamp_ms();
    if (ms > now) {
        sleep_for_ms(ms - now);
    }
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
#endif

} // namespace core
} // namespace roc
