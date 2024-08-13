/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/mutex.h
//! @brief Mutex.

#ifndef ROC_CORE_MUTEX_H_
#define ROC_CORE_MUTEX_H_

#include "roc_core/atomic.h"
#include "roc_core/attributes.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/scoped_lock.h"

#include <errno.h>
#include <pthread.h>

namespace roc {
namespace core {

class Cond;

//! Mutex.
class Mutex : public NonCopyable<> {
public:
    //! RAII lock.
    typedef ScopedLock<Mutex> Lock;

    //! Initialize mutex.
    Mutex();

    //! Destroy mutex.
    ~Mutex();

    //! Try to lock the mutex.
    ROC_ATTR_NODISCARD inline bool try_lock() const {
        const int err = pthread_mutex_trylock(&mutex_);

        if (err != 0 && err != EBUSY && err != EAGAIN) {
            roc_panic("mutex: pthread_mutex_trylock(): %s", errno_to_str(err).c_str());
        }

        return (err == 0);
    }

    //! Lock mutex.
    inline void lock() const {
        if (int err = pthread_mutex_lock(&mutex_)) {
            roc_panic("mutex: pthread_mutex_lock(): %s", errno_to_str(err).c_str());
        }
    }

    //! Unlock mutex.
    inline void unlock() const {
        ++guard_;

        if (int err = pthread_mutex_unlock(&mutex_)) {
            roc_panic("mutex: pthread_mutex_unlock(): %s", errno_to_str(err).c_str());
        }

        --guard_;
    }

private:
    friend class Cond;

    mutable pthread_mutex_t mutex_;
    mutable Atomic<int> guard_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MUTEX_H_
