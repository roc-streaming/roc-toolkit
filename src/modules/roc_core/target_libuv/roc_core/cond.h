/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libuv/roc_core/cond.h
//! @brief Condition variable.

#ifndef ROC_CORE_COND_H_
#define ROC_CORE_COND_H_

#include <uv.h>

#include "roc_core/atomic.h"
#include "roc_core/cpu_ops.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Condition variable.
class Cond : public NonCopyable<> {
public:
    //! Initialize.
    Cond(const Mutex& mutex)
        : guard_(0)
        , mutex_(mutex.mutex_) {
        if (int err = uv_cond_init(&cond_)) {
            roc_panic("cond: uv_cond_init(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    ~Cond() {
        while (guard_) {
            cpu_relax();
        }
        uv_cond_destroy(&cond_);
    }

    //! Wait with timeout.
    //! @returns false if timeout expired.
    bool timed_wait(nanoseconds_t timeout) const {
        const int err = uv_cond_timedwait(&cond_, &mutex_, (uint64_t)timeout);
        if (err != 0 && err != UV_ETIMEDOUT) {
            roc_panic("cond: uv_cond_timedwait(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
        return (err == 0);
    }

    //! Wait.
    void wait() const {
        uv_cond_wait(&cond_, &mutex_);
    }

    //! Wake up one pending waits.
    void signal() const {
        ++guard_;
        uv_cond_signal(&cond_);
        --guard_;
    }

    //! Wake up all pending waits.
    void broadcast() const {
        ++guard_;
        uv_cond_broadcast(&cond_);
        --guard_;
    }

private:
    mutable uv_cond_t cond_;
    mutable Atomic<int> guard_;

    uv_mutex_t& mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_COND_H_
