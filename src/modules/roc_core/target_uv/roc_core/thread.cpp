/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/thread.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

Thread::Thread()
    : joinable_(false) {
}

Thread::~Thread() {
    if (joinable_) {
        roc_panic("thread was not joined before calling thread object destructor");
    }
}

bool Thread::joinable() const {
    return joinable_;
}

void Thread::start() {
    if (joinable_) {
        roc_panic("attempting to start thread that is already running");
    }

    if (int err = uv_thread_create(&thread_, thread_runner_, this)) {
        roc_panic("uv_thread_create(): [%s] %s", uv_err_name(err), uv_strerror(err));
    }

    joinable_ = true;
}

void Thread::join() {
    if (!joinable_) {
        roc_panic("attempting to join thread that is not started");
    }

    if (int err = uv_thread_join(&thread_)) {
        roc_panic("uv_thread_join(): [%s] %s", uv_err_name(err), uv_strerror(err));
    }

    joinable_ = false;
}

void Thread::thread_runner_(void* ptr) {
    static_cast<Thread*>(ptr)->run();
}

} // namespace core
} // namespace roc
