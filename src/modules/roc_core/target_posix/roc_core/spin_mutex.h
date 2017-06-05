/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/spin_mutex.h
//! @brief Spinlock mutex.

#ifndef ROC_CORE_SPIN_MUTEX_H_
#define ROC_CORE_SPIN_MUTEX_H_

#include <pthread.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/scoped_lock.h"

namespace roc {
namespace core {

//! Spinlock mutex.
class SpinMutex : public NonCopyable<> {
public:
    //! Scoped lock.
    typedef ScopedLock<SpinMutex> Lock;

    SpinMutex() {
        int err = pthread_spin_init(&spinlock_, 0);
        if (err != 0) {
            roc_panic("pthread_spin_init(): %s", errno_to_str(err).c_str());
        }
    }

    ~SpinMutex() {
        int err = pthread_spin_destroy(&spinlock_);
        if (err != 0) {
            roc_panic("pthread_mutex_destroy(): %s", errno_to_str(err).c_str());
        }
    }

    //! Lock mutex.
    void lock() const {
        int err = pthread_spin_lock(&spinlock_);
        if (err != 0) {
            roc_panic("pthread_spin_lock(): %s", errno_to_str(err).c_str());
        }
    }

    //! Unlock mutex.
    void unlock() const {
        int err = pthread_spin_unlock(&spinlock_);
        if (err != 0) {
            roc_panic("pthread_spin_unlock(): %s", errno_to_str(err).c_str());
        }
    }

private:
    mutable pthread_spinlock_t spinlock_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SPIN_MUTEX_H_
