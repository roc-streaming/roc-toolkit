/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/semaphore.h"
#include "roc_core/cpu_instructions.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#include <errno.h>
#include <time.h>

namespace roc {
namespace core {

Semaphore::Semaphore(unsigned counter)
    : mutex_()
    , counter_(counter)
    , guard_(0) {
    pthread_condattr_t attr;

    if (int err = pthread_condattr_init(&attr)) {
        roc_panic("semaphore: pthread_condattr_init(): %s", errno_to_str(err).c_str());
    }

#if defined(CLOCK_MONOTONIC) && !defined(__APPLE__) && !defined(__MACH__)
    if (int err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
        roc_panic("semaphore: pthread_condattr_setclock(): %s",
                  errno_to_str(err).c_str());
    }
#endif

    if (int err = pthread_cond_init(&cond_, &attr)) {
        roc_panic("semaphore: pthread_cond_init(): %s", errno_to_str(err).c_str());
    }

    if (int err = pthread_condattr_destroy(&attr)) {
        roc_panic("semaphore: pthread_condattr_destroy(): %s", errno_to_str(err).c_str());
    }
}

Semaphore::~Semaphore() {
    /* Ensure that signal() and broadcast() are not using condvar.
     */
    while (guard_) {
        cpu_relax();
    }

    int err;

#if defined(__APPLE__) && defined(__MACH__)
    if ((err = pthread_mutex_lock(&mutex_.mutex_))) {
        roc_panic("mutex: pthread_mutex_lock(): %s", errno_to_str(err).c_str());
    }

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1;

    err = pthread_cond_timedwait_relative_np(&cond_, &mutex_.mutex_, &ts);
    if (err != 0 && err != ETIMEDOUT) {
        roc_panic("mutex: pthread_cond_timedwait_relative_np(): %s",
                  errno_to_str(err).c_str());
    }

    if ((err = pthread_mutex_unlock(&mutex_.mutex_))) {
        roc_panic("mutex: pthread_mutex_unlock(): %s", errno_to_str(err).c_str());
    }
#endif

    if ((err = pthread_cond_destroy(&cond_))) {
        roc_panic("sem: pthread_cond_destroy(): %s", errno_to_str(err).c_str());
    }
}

bool Semaphore::timed_wait(nanoseconds_t deadline) {
    if (deadline < 0) {
        roc_panic("semaphore: unexpected negative deadline");
    }

    struct timespec ts;
    int err = 0;

    mutex_.lock();
    while (err == 0 && counter_ == 0) {
#if defined(__APPLE__) && defined(__MACH__)
        // On macOS, convert absolute deadline to relative timeout
        nanoseconds_t now = timestamp(ClockMonotonic);
        if (deadline <= now) {
            err = ETIMEDOUT;
            break;
        }
        nanoseconds_t timeout = deadline - now;
        
        ts.tv_sec = time_t(timeout / Second);
        ts.tv_nsec = long(timeout % Second);
        
        err = pthread_cond_timedwait_relative_np(&cond_, &mutex_.mutex_, &ts);
#else
        ts.tv_sec = time_t(deadline / Second);
        ts.tv_nsec = long(deadline % Second);
        
        err = pthread_cond_timedwait(&cond_, &mutex_.mutex_, &ts);
#endif
    }

    bool acquired = (counter_ > 0);
    if (acquired) {
        counter_--;
    }
    mutex_.unlock();

    if (err != 0 && err != ETIMEDOUT) {
        roc_panic("semaphore: pthread_cond_timedwait(): %s", errno_to_str(err).c_str());
    }

    return acquired;
}

void Semaphore::wait() {
    int err = 0;
    mutex_.lock();
    while (err == 0 && counter_ == 0) {
        if ((err = pthread_cond_wait(&cond_, &mutex_.mutex_))) {
            roc_panic("semaphore: pthread_cond_wait(): %s", errno_to_str(err).c_str());
        }
    }
    counter_--;
    mutex_.unlock();
}

void Semaphore::post() {
    ++guard_;
    mutex_.lock();
    counter_++;
    if (int err = pthread_cond_broadcast(&cond_)) {
        roc_panic("cond: pthread_cond_broadcast(): %s", errno_to_str(err).c_str());
    }
    mutex_.unlock();
    --guard_;
}

} // namespace core
} // namespace roc