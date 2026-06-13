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

#include <windows.h>

namespace roc {
namespace core {

Semaphore::Semaphore(unsigned counter)
    : guard_(0) {
    sem_ = CreateSemaphore(NULL, (LONG)counter, INT_MAX, NULL);
    if (!sem_) {
        roc_panic("semaphore: CreateSemaphore(): %s", errno_to_str().c_str());
    }
}

Semaphore::~Semaphore() {
    // TODO: faut-il release ici avant le CloseHandle?
    while (guard_) {
        cpu_relax();
    }
    if (!CloseHandle(sem_)) {
        roc_panic("semaphore: CloseHandle(): %s", errno_to_str().c_str());
    }
}

bool Semaphore::timed_wait(nanoseconds_t deadline) {
    if (deadline < 0) {
        roc_panic("semaphore: unexpected negative deadline");
    }

    DWORD deadline_ms = deadline / 1000000;
    if (deadline && !deadline_ms) {
        deadline_ms = 1;
    }

    DWORD wr = WaitForSingleObject(sem_, deadline_ms);

    switch (wr) {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        return false;
    case WAIT_FAILED:
        roc_panic("semaphore: WaitForSingleObject(): %s", errno_to_str().c_str());
    default:
        roc_panic("semaphore: WaitForSingleObject(): unexpected return %lu", wr);
    }
}

void Semaphore::wait() {
    DWORD wr = WaitForSingleObject(sem_, INFINITE);

    switch (wr) {
    case WAIT_OBJECT_0:
        return;
    case WAIT_FAILED:
        roc_panic("semaphore: WaitForSingleObject(): %s", errno_to_str().c_str());
    default:
        roc_panic("semaphore: WaitForSingleObject(): unexpected return %lu", wr);
    }
}

void Semaphore::post() {
    ++guard_;
    if (!ReleaseSemaphore(sem_, 1, NULL)) {
        roc_panic("semaphore: ReleaseSemaphore(): %s", errno_to_str().c_str());
    }
    --guard_;
}

} // namespace core
} // namespace roc
