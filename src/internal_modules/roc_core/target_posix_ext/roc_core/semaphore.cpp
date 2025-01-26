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
#include "roc_core/panic.h"
#include "roc_core/log.h"

#include <errno.h>
#include <time.h>

#ifdef sem_clockwait
#define HAS_SEM_CLOCKWAIT 1
#else
#define HAS_SEM_CLOCKWAIT 0
#endif

namespace roc {
namespace core {

Semaphore::Semaphore(unsigned counter)
    : guard_(0) {
    if (sem_init(&sem_, 0, counter) != 0) {
        roc_panic("semaphore: sem_init(): %s", errno_to_str().c_str());
    }
}

Semaphore::~Semaphore() {
    while (guard_) {
        cpu_relax();
    }
    if (sem_destroy(&sem_) != 0) {
        roc_panic("semaphore: sem_destroy(): %s", errno_to_str().c_str());
    }
}

bool Semaphore::timed_wait(nanoseconds_t deadline) {

    if (deadline < 0) {
        roc_panic("semaphore: unexpected negative deadline");
    }

    nanoseconds_t converted_deadline = deadline;

    for (;;) {
        
        // WHAT IF CHANGE SYSTEM TIME WHEN -1?
        if (!HAS_SEM_CLOCKWAIT) {
            converted_deadline = deadline + core::timestamp(core::ClockUnix) - core::timestamp(core::ClockMonotonic);
        }

        timespec ts;
        ts.tv_sec = long(converted_deadline / Second);
        ts.tv_nsec = long(converted_deadline % Second);
        roc_log(LogDebug,"time to wait: %li",ts.tv_sec);

        if (HAS_SEM_CLOCKWAIT) {
            if (sem_clockwait(&sem_, core::ClockMonotonic ,&ts) == 0) {
                return true;
            }
        } else {
            if (sem_timedwait(&sem_, &ts) == 0) {
                return true;
            }
        }


        if (errno == ETIMEDOUT) {
            return false;
        }

        if (errno != EINTR) {
            roc_panic("semaphore: sem_wait(): %s", errno_to_str().c_str());
        }
    }
}

void Semaphore::wait() {
    for (;;) {
        if (sem_wait(&sem_) == 0) {
            return;
        } 
        if (errno != EINTR) {
            roc_panic("semaphore: sem_wait(): %s", errno_to_str().c_str());
        }
    }
}

void Semaphore::post() {
    ++guard_;
    for (;;) {
        if (sem_post(&sem_) == 0) {
            break;
        }
        if (errno != EINTR) {
            roc_panic("semaphore: sem_post(): %s", errno_to_str().c_str());
        }
    }
    --guard_;
}

} // namespace core
} // namespace roc
