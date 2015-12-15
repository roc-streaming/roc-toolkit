/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_uv/roc_core/semaphore.h
//! @brief Semaphore.

#ifndef ROC_CORE_SEMAPHORE_H_
#define ROC_CORE_SEMAPHORE_H_

#include <uv.h>

#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Semaphore.
class Semaphore : NonCopyable<> {
public:
    //! Initialize semaphore with given counter value.
    explicit Semaphore(size_t counter = 0) {
        if (int err = uv_sem_init(&sem_, (unsigned int)counter)) {
            roc_panic("uv_sem_init(): [%s] %s", uv_err_name(err), uv_strerror(err));
        }
    }

    ~Semaphore() {
        uv_sem_destroy(&sem_);
    }

    //! Decrement semaphore counter.
    //! @remarks
    //!  Blocks until counter will be greather than zero.
    void pend() {
        uv_sem_wait(&sem_);
    }

    //! Increment semaphore counter.
    //! @remarks
    //!  Notifies threads that are blocked on pend().
    void post() {
        uv_sem_post(&sem_);
    }

private:
    uv_sem_t sem_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEMAPHORE_H_
