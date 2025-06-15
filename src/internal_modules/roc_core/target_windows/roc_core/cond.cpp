/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/cond.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

Cond::Cond(const Mutex& mutex)
    : mutex_(mutex.mutex_) {
    InitializeConditionVariable(&cond_);
}

Cond::~Cond() {
    // no-op
}

bool Cond::timed_wait(nanoseconds_t timeout) const {
    DWORD timeout_ms = (DWORD)(timeout / 1000000);
    if (timeout_ms == 0 && timeout > 0) {
        timeout_ms = 1;
    }

    const BOOL result = SleepConditionVariableCS(&cond_, &mutex_, timeout_ms);

    if (!result) {
        const DWORD err = GetLastError();
        if (err == ERROR_TIMEOUT || err == WAIT_TIMEOUT) {
            return false;
        }
        roc_panic("cond: SleepConditionVariableCS(): error %lu", err);
    }

    return true;
}

void Cond::wait() const {
    if (!SleepConditionVariableCS(&cond_, &mutex_, INFINITE)) {
        DWORD error = GetLastError();
        roc_panic("cond: SleepConditionVariableCS(): error %lu", error);
    }
}

void Cond::signal() const {
    WakeConditionVariable(&cond_);
}

void Cond::broadcast() const {
    WakeAllConditionVariable(&cond_);
}

} // namespace core
} // namespace roc
