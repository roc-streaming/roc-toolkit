/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libuv/roc_core/mutex.h
//! @brief Mutex.

#ifndef ROC_CORE_MUTEX_H_
#define ROC_CORE_MUTEX_H_

#include <uv.h>

#include "roc_core/atomic.h"
#include "roc_core/cpu_ops.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/scoped_lock.h"

namespace roc {
namespace core {

class Cond;

//! Mutex.
class Mutex : public NonCopyable<> {
public:
    //! RAII lock.
    typedef ScopedLock<Mutex> Lock;

    Mutex()
        : guard_(0) {
        if (int err = uv_mutex_init(&mutex_)) {
            roc_panic("mutex: uv_mutex_init(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    ~Mutex() {
        while (guard_) {
            cpu_relax();
        }
        uv_mutex_destroy(&mutex_);
    }

    //! Try to lock the mutex.
    bool try_lock() const {
        return uv_mutex_trylock(&mutex_) == 0;
    }

    //! Lock mutex.
    void lock() const {
        uv_mutex_lock(&mutex_);
    }

    //! Unlock mutex.
    void unlock() const {
        ++guard_;
        uv_mutex_unlock(&mutex_);
        --guard_;
    }

private:
    friend class Cond;

    mutable uv_mutex_t mutex_;
    mutable Atomic<int> guard_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MUTEX_H_
