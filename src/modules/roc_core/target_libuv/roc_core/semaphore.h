/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libuv/roc_core/semaphore.h
//! @brief Semaphore.

#ifndef ROC_CORE_SEMAPHORE_H_
#define ROC_CORE_SEMAPHORE_H_

#include <uv.h>

#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Semaphore.
class Semaphore : public NonCopyable<> {
public:
    //! Initialize semaphore with given counter.
    Semaphore(unsigned counter = 0) {
        if (int err = uv_sem_init(&sem_, counter)) {
            roc_panic("semaphore: uv_sem_init(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    ~Semaphore() {
        uv_sem_destroy(&sem_);
    }

    //! Block until the counter becomes non-zero and decrement it.
    void wait() const {
        uv_sem_wait(&sem_);
    }

    //! Increment counter and wake up blocked pends.
    void post() const {
        uv_sem_post(&sem_);
    }

private:
    mutable uv_sem_t sem_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEMAPHORE_H_
