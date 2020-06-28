/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"
#include "roc_core/timer.h"

namespace {

struct itimerspec infinity_deadline() {
    struct itimerspec new_value;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    new_value.it_value.tv_sec = 0;
    new_value.it_value.tv_nsec = 0;

    return new_value;
}

struct itimerspec immediately_deadline() {
    struct itimerspec new_value;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    new_value.it_value.tv_sec = 0;
    new_value.it_value.tv_nsec = roc::core::Nanosecond;

    return new_value;
}

struct itimerspec finity_deadline(roc::core::nanoseconds_t deadline) {
    struct itimerspec new_value;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    new_value.it_value.tv_sec = (time_t)(deadline / roc::core::Second);
    new_value.it_value.tv_nsec = deadline % roc::core::Second;

    return new_value;
}

struct itimerspec convert_deadline(roc::core::nanoseconds_t deadline) {
    if (deadline < 0) {
        return infinity_deadline();
    } else if (deadline == 0) {
        return immediately_deadline();
    } else {
        return finity_deadline(deadline);
    }
}

} // namespace anonymous

namespace roc {
namespace core {

Timer::Timer()
    : timerfd_(timerfd_create(CLOCK_MONOTONIC, 0))
    , deadline_(0)
    , is_waiting_(false) {
    if (timerfd_ == -1) {
        roc_panic("timer: timerfd_create(): %s", errno_to_str().c_str());
    }
}

Timer::~Timer() {
    int res = close(timerfd_);
    if (res == -1) {
        roc_panic("timer: close(): %s", errno_to_str().c_str());
    }
}

bool Timer::try_set_deadline(nanoseconds_t new_deadline) {
    if (!deadline_.try_store(new_deadline)) {
        return false;
    }

    if (is_waiting_.wait_load()) {
        syscall_set(new_deadline);
    }

    return true;
}

void Timer::wait_deadline() {
    /*if (is_waiting_.wait_load()) {
        return;
    }*/

    const nanoseconds_t deadline = deadline_.wait_load();

    if (deadline >= 0 && deadline <= timestamp()) {
        return;
    }

    is_waiting_.try_store(true);
    syscall_set(deadline);
    syscall_wait();
    is_waiting_.try_store(false);
}

void Timer::syscall_set(nanoseconds_t deadline) {
    struct itimerspec new_value = convert_deadline(deadline);

    int res = timerfd_settime(timerfd_, TFD_TIMER_ABSTIME, &new_value, NULL);
    if (res == -1) {
        roc_panic("timer: timerfd_settime(): %s", errno_to_str().c_str());
    }
}

void Timer::syscall_wait() {
    uint64_t ticks = 0;
    ssize_t readed = -1;

    do {
        readed = read(timerfd_, &ticks, sizeof(uint64_t));
    } while (readed == -1 && errno == EINTR);

    if (readed == -1 || ticks == 0) {
        roc_panic("timer: read(): %s", errno_to_str().c_str());
    }
}

} // namespace core
} // namespace roc
