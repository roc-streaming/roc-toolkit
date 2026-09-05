/*
 * Copyright (c) 2026 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/semaphore.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

Semaphore::Semaphore(unsigned counter)
    : cond_(mutex_)
    , counter_(counter) {
}

bool Semaphore::timed_wait(nanoseconds_t deadline) {
    if (deadline < 0) {
        roc_panic("semaphore: unexpected negative deadline");
    }

    Mutex::Lock lock(mutex_);

    while (counter_ == 0) {
        const nanoseconds_t timeout = deadline - timestamp(ClockMonotonic);
        if (timeout <= 0) {
            return false;
        }
        (void)cond_.timed_wait(timeout);
    }

    counter_--;

    return true;
}

void Semaphore::wait() {
    Mutex::Lock lock(mutex_);

    while (counter_ == 0) {
        cond_.wait();
    }

    counter_--;
}

void Semaphore::post() {
    Mutex::Lock lock(mutex_);

    counter_++;
    cond_.signal();
}

} // namespace core
} // namespace roc
