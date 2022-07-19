/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/mutex.h"
#include "roc_core/cpu_instructions.h"

namespace roc {
namespace core {

Mutex::Mutex()
    : guard_(0) {
    pthread_mutexattr_t attr;

    if (int err = pthread_mutexattr_init(&attr)) {
        roc_panic("mutex: pthread_mutexattr_init(): %s", errno_to_str(err).c_str());
    }

    if (int err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK)) {
        roc_panic("mutex: pthread_mutexattr_settype(): %s", errno_to_str(err).c_str());
    }

    if (int err = pthread_mutex_init(&mutex_, &attr)) {
        roc_panic("mutex: pthread_mutex_init(): %s", errno_to_str(err).c_str());
    }

    if (int err = pthread_mutexattr_destroy(&attr)) {
        roc_panic("mutex: pthread_mutexattr_destroy(): %s", errno_to_str(err).c_str());
    }
}

Mutex::~Mutex() {
    /* Ensure that unlock() is not using condvar.
     */
    while (guard_) {
        cpu_relax();
    }

    if (int err = pthread_mutex_destroy(&mutex_)) {
        roc_panic("mutex: pthread_mutex_destroy(): %s", errno_to_str(err).c_str());
    }
}

} // namespace core
} // namespace roc
