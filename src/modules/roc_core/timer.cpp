/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/timer.h"

namespace roc {
namespace core {

Timer::Timer()
    : cond_(mutex_)
    , deadline_(0)
    , next_wakeup_(0) {
}

void Timer::set_deadline(nanoseconds_t deadline) {
    {
        Mutex::Lock lock(mutex_);

        if (deadline_ == deadline) {
            return;
        }

        deadline_ = deadline;

        if (next_wakeup_ == deadline || (next_wakeup_ >= 0 && next_wakeup_ <= deadline)) {
            return;
        }
    }

    cond_.broadcast();
}

void Timer::wait_deadline() {
    Mutex::Lock lock(mutex_);

    for (;;) {
        const nanoseconds_t now = timestamp();

        next_wakeup_ = deadline_;

        if (deadline_ < 0) {
            cond_.wait();
            continue;
        }

        if (deadline_ > now) {
            cond_.timed_wait(deadline_ - now);
            continue;
        }

        return;
    }
}

} // namespace core
} // namespace roc
