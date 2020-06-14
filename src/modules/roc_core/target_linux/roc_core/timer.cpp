/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <unistd.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"
#include "roc_core/timer.h"

namespace roc {
namespace core {

#if defined(CLOCK_MONOTONIC)
#define CLOCK_TYPE CLOCK_MONOTONIC
#else
#define CLOCK_TYPE CLOCK_REALTIME
#endif

Timer::Timer()
    : timerfd_(timerfd_create(CLOCK_TYPE, 0))
    , deadline_(0) {
    if (timerfd_ == -1) {
        roc_panic("timer: timerfd_create(CLOCK_TYPE, 0): %s", errno_to_str().c_str());
    }
}

Timer::~Timer() {
    close(timerfd_);
}

bool Timer::try_set_deadline(nanoseconds_t new_deadline) {
    if (!deadline_.try_store(new_deadline)) {
        return false;
    }

    struct itimerspec new_value = convert_deadline();

    int res = timerfd_settime(timerfd_, TFD_TIMER_ABSTIME, &new_value, NULL);
    if (res == -1) {
        roc_panic("timer: timerfd_settime(timerfd_, TFD_TIMER_ABSTIME, "
                  "&new_value, NULL): %s",
                  errno_to_str().c_str());
    }

    return true;
}

void Timer::wait_deadline() {
    const nanoseconds_t deadline = deadline_.wait_load();

    if (deadline >= 0 && deadline <= timestamp()) {
        return;
    }

    uint64_t ticks = 0;
    ssize_t readed = read(timerfd_, &ticks, sizeof(uint64_t));
    if (readed == -1 || ticks == 0) {
        roc_panic("timer: read(timerfd_, &ticks, sizeof(uint64_t)): %s",
                  errno_to_str().c_str());
    }
}

struct itimerspec Timer::convert_deadline() {
    const nanoseconds_t deadline = deadline_.wait_load();

    if (deadline < 0) {
        return infinity_deadline();
    } else if (deadline == 0) {
        return immediately_deadline();
    } else {
        return finity_deadline();
    }
}

struct itimerspec Timer::infinity_deadline() {
    struct itimerspec new_value = {};

    return new_value;
}

struct itimerspec Timer::immediately_deadline() {
    struct itimerspec new_value = {};
    new_value.it_value.tv_nsec = Nanosecond;

    return new_value;
}

struct itimerspec Timer::finity_deadline() {
    const nanoseconds_t deadline = deadline_.wait_load();

    struct itimerspec new_value = {};
    new_value.it_value.tv_sec = (time_t)(deadline / Second);
    new_value.it_value.tv_nsec = deadline % Second;

    return new_value;
}

} // namespace core
} // namespace roc
