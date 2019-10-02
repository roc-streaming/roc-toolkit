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

#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Condition variable.
class Cond : public NonCopyable<> {
public:
    //! Initialize.
    Cond(const Mutex& mutex)
        : mutex_(mutex.mutex_) {
        if (int err = uv_cond_init(&cond_)) {
            roc_panic("cond: uv_cond_init(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    ~Cond() {
        uv_cond_destroy(&cond_);
    }

    //! Wait.
    void wait() const {
        uv_cond_wait(&cond_, &mutex_);
    }

    //! Wake up all pending waits.
    void broadcast() const {
        uv_cond_broadcast(&cond_);
    }

private:
    mutable uv_cond_t cond_;
    uv_mutex_t& mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_COND_H_
