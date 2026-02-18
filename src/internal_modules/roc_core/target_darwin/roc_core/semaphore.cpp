/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/semaphore.h"
#include "roc_core/panic.h"

#include <mach/mach.h>
#include <mach/sync_policy.h>

namespace roc {
namespace core {

Semaphore::Semaphore(unsigned counter) {
    const kern_return_t ret =
        semaphore_create(mach_task_self(), &sem_id_, SYNC_POLICY_FIFO, (int)counter);

    if (ret != KERN_SUCCESS) {
        roc_panic("semaphore: semaphore_create(): %s", mach_error_string(ret));
    }
}

Semaphore::~Semaphore() {
    const kern_return_t ret = semaphore_destroy(mach_task_self(), sem_id_);

    if (ret != KERN_SUCCESS) {
        roc_panic("semaphore: semaphore_destroy(): %s", mach_error_string(ret));
    }
}

bool Semaphore::timed_wait(nanoseconds_t deadline) {
    if (deadline < 0) {
        roc_panic("semaphore: unexpected negative deadline");
    }

    for (;;) {
        const nanoseconds_t timeout = deadline - timestamp(ClockMonotonic);
        if (timeout <= 0) {
            return false;
        }

        mach_timespec_t ts;
        ts.tv_sec = unsigned(timeout / Second);
        ts.tv_nsec = timeout % Second;

        const kern_return_t ret = semaphore_timedwait(sem_id_, ts);

        if (ret == KERN_SUCCESS) {
            return true;
        }

        if (ret == KERN_OPERATION_TIMED_OUT) {
            return false;
        }

        if (ret != KERN_ABORTED) {
            roc_panic("semaphore: semaphore_wait(): %s", mach_error_string(ret));
        }
    }
}

void Semaphore::wait() {
    for (;;) {
        const kern_return_t ret = semaphore_wait(sem_id_);

        if (ret == KERN_SUCCESS) {
            return;
        }

        if (ret != KERN_ABORTED) {
            roc_panic("semaphore: semaphore_wait(): %s", mach_error_string(ret));
        }
    }
}

void Semaphore::post() {
    const kern_return_t ret = semaphore_signal(sem_id_);

    if (ret != KERN_SUCCESS) {
        roc_panic("semaphore: semaphore_post(): %s", mach_error_string(ret));
    }
}

} // namespace core
} // namespace roc
