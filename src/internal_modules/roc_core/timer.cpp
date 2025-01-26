/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/timer.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

Timer::Timer()
    : sem_(0)
    , sem_post_flag_(false)
    , deadline_(0)
    , next_wakeup_(0) {
}

bool Timer::try_set_deadline(nanoseconds_t new_deadline) {
    if (!deadline_.try_store(new_deadline)) {
        return false;
    }

    nanoseconds_t next_wakeup;

    if (!next_wakeup_.try_load(next_wakeup)) {
        next_wakeup = -1;
    }

    // if 1. new deadline is earlier than the scheduled wakeup time; or 2. nextwakeup <0 ,so timer is not active
    // post only if sem flag is not set (to aviod duplicate signaling)
    if (next_wakeup < 0 || (new_deadline >= 0 && new_deadline < next_wakeup)) {
        if (sem_post_flag_.compare_exchange(false, true)) {
            sem_.post();
        }
    }

    return true;
}

void Timer::wait_deadline() {
    for (;;) {
        // set a lock on next_wakeup?
        next_wakeup_.exclusive_store(-1);

        const nanoseconds_t deadline = deadline_.wait_load();

        // continue if ddl is less than a clock (just because input is less, ddl is never decremented.)
        if (deadline >= 0 && deadline <= timestamp(ClockMonotonic)) {
            break;
        }

        // wait forever if no deadline (will wakeup when other people set deadline)
        // stuck here until sem timeout if have deadline (will I wake up if other people wake sem?) (do other people come to this statement too?)
        if (deadline > 0) {
            next_wakeup_.exclusive_store(deadline);
            (void)sem_.timed_wait(deadline);
        } else {
            sem_.wait();
        }

        sem_post_flag_ = false;
    }

    //release the lock
    next_wakeup_.exclusive_store(0);
}

} // namespace core
} // namespace roc
