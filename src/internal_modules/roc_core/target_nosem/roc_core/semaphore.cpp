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
    if (int err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
        roc_panic("semaphore: pthread_condattr_setclock(): %s", errno_to_str(err).c_str());
    }
    if (int err = pthread_cond_init(&cond_, &attr)) {
        roc_panic("semaphore: pthread_cond_init(): %s", errno_to_str(err).c_str());
    }

    if (int err = pthread_condattr_destroy(&attr)) {
        roc_panic("semaphore: pthread_condattr_destroy(): %s", errno_to_str(err).c_str());
    }
}

Semaphore::~Semaphore() {
    while (guard_) {
        cpu_relax();
    }

    int err = 0;
    if ((err = pthread_cond_destroy(&cond_))) {
        roc_panic("sem: pthread_cond_destroy(): %s", errno_to_str(err).c_str());
    }
}

bool Semaphore::timed_wait(nanoseconds_t deadline) {
    if (deadline < 0) {
        roc_panic("semaphore: unexpected negative deadline");
    }

    struct timespec ts;
    ts.tv_sec = time_t(deadline / Second);
    ts.tv_nsec = long(deadline % Second);
    // roc_log(LogDebug, "loop top, now=%lld, deadline=%lld",
    // core::timestamp(core::ClockMonotonic), deadline);

    int err = 0;
    mutex_.lock();
    while (err == 0 && counter_ == 0) {
        err = pthread_cond_timedwait(&cond_, &mutex_.mutex_, &ts);
    }

    bool acquired = (counter_ > 0);
    if (acquired) {
        counter_--;
    }
    mutex_.unlock();

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