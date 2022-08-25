/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/cond.h"
#include "roc_core/cpu_instructions.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Initialize.
Cond::Cond(const Mutex& mutex)
    : guard_(0)
    , mutex_(mutex.mutex_) {
    pthread_condattr_t attr;

    if (int err = pthread_condattr_init(&attr)) {
        roc_panic("cond: pthread_condattr_init(): %s", errno_to_str(err).c_str());
    }

#if defined(CLOCK_MONOTONIC) && !defined(__APPLE__) && !defined(__MACH__)
    if (int err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
        roc_panic("cond: pthread_condattr_setclock(): %s", errno_to_str(err).c_str());
    }
#endif

    if (int err = pthread_cond_init(&cond_, &attr)) {
        roc_panic("cond: pthread_cond_init(): %s", errno_to_str(err).c_str());
    }

    if (int err = pthread_condattr_destroy(&attr)) {
        roc_panic("cond: pthread_condattr_destroy(): %s", errno_to_str(err).c_str());
    }
}

Cond::~Cond() {
    /* Ensure that signal() and broadcast() are not using condvar.
     */
    while (guard_) {
        cpu_relax();
    }

    int err;

#if defined(__APPLE__) && defined(__MACH__)
    /* Ensure that condvar is waited before destroying it.
     * https://codereview.chromium.org/1323293005
     */
    if ((err = pthread_mutex_lock(&mutex_))) {
        roc_panic("mutex: pthread_mutex_lock(): %s", errno_to_str(err).c_str());
    }

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1;

    err = pthread_cond_timedwait_relative_np(&cond_, &mutex_, &ts);
    if (err != 0 && err != ETIMEDOUT) {
        roc_panic("mutex: pthread_cond_timedwait_relative_np(): %s",
                  errno_to_str(err).c_str());
    }

    if ((err = pthread_mutex_unlock(&mutex_))) {
        roc_panic("mutex: pthread_mutex_unlock(): %s", errno_to_str(err).c_str());
    }
#endif

    if ((err = pthread_cond_destroy(&cond_))) {
        roc_panic("cond: pthread_cond_destroy(): %s", errno_to_str(err).c_str());
    }
}

bool Cond::timed_wait(nanoseconds_t timeout) const {
    struct timespec ts;
    int err;

#if defined(__APPLE__) && defined(__MACH__)
    ts.tv_sec = time_t(timeout / Second);
    ts.tv_nsec = long(timeout % Second);

    err = pthread_cond_timedwait_relative_np(&cond_, &mutex_, &ts);
#elif defined(CLOCK_MONOTONIC)
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        roc_panic("cond: clock_gettime(): %s", errno_to_str().c_str());
    }

    timeout += (nanoseconds_t)ts.tv_sec * Second + (nanoseconds_t)ts.tv_nsec;

    ts.tv_sec = time_t(timeout / Second);
    ts.tv_nsec = long(timeout % Second);

    err = pthread_cond_timedwait(&cond_, &mutex_, &ts);
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        roc_panic("cond: clock_gettime(): %s", errno_to_str().c_str());
    }

    timeout +=
        (nanoseconds_t)ts.tv_sec * Second + (nanoseconds_t)ts.tv_usec * Microsecond;

    ts.tv_sec = time_t(timeout / Second);
    ts.tv_nsec = long(timeout % Second);

    err = pthread_cond_timedwait(&cond_, &mutex_, &ts);
#endif

    if (err != 0 && err != ETIMEDOUT) {
        roc_panic("cond: pthread_cond_timedwait(): %s", errno_to_str(err).c_str());
    }

    return (err == 0);
}

void Cond::wait() const {
    if (int err = pthread_cond_wait(&cond_, &mutex_)) {
        roc_panic("cond: pthread_cond_wait(): %s", errno_to_str(err).c_str());
    }
}

void Cond::signal() const {
    ++guard_;

    if (int err = pthread_cond_signal(&cond_)) {
        roc_panic("cond: pthread_cond_signal(): %s", errno_to_str(err).c_str());
    }

    --guard_;
}

void Cond::broadcast() const {
    ++guard_;

    if (int err = pthread_cond_broadcast(&cond_)) {
        roc_panic("cond: pthread_cond_broadcast(): %s", errno_to_str(err).c_str());
    }

    --guard_;
}

} // namespace core
} // namespace roc
