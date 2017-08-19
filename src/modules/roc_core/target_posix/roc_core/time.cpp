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

uint64_t timestamp_ms() {
    clock_serv_t cclock;
    mach_timespec_t mts;

    // Receive a permission to send a messages to a specified clock service.
    kern_return_t ret =
        host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    if (ret != KERN_SUCCESS) {
        roc_panic("host_get_clock_service: %d", ret);
    }

    // man for a clock_get_time promises that it increments monotonically.
    // https://opensource.apple.com/source/xnu/xnu-2422.1.72/osfmk/man/clock_get_time.html
    //
    // But if you really want to call clock_set_time, kernel will return an
    // error for it.
    // https://opensource.apple.com/source/xnu/xnu-2422.1.72/osfmk/man/clock_get_time.html
    ret = clock_get_time(cclock, &mts);
    if (ret != KERN_SUCCESS) {
        roc_panic("clock_get_time(CLOCK_MONOTONIC): %d", ret);
    }

    ret = mach_port_deallocate(mach_task_self(), cclock);
    if (ret != KERN_SUCCESS) {
        roc_panic("mach_port_deallocate: %d", ret);
    }

    return uint64_t(mts.tv_sec) * 1000 + uint64_t(mts.tv_nsec) / 1000000;
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
