/*
 * Copyright (c) 2026 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_nosem/roc_core/semaphore.h
//! @brief Semaphore.

#ifndef ROC_CORE_SEMAPHORE_H_
#define ROC_CORE_SEMAPHORE_H_

#include "roc_core/attributes.h"
#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Semaphore.
//!
//! @remarks
//!  This implementation is used on platforms that don't provide sem_clockwait(),
//!  and hence can't wait on a POSIX semaphore using monotonic clock. It is based
//!  on mutex and condition variable. Unlike other implementations, post() is not
//!  lock-free here.
class Semaphore : public NonCopyable<> {
public:
    //! Initialize semaphore with given counter.
    explicit Semaphore(unsigned counter = 0);

    //! Wait until the counter becomes non-zero, decrement it, and return true.
    //! If deadline expires before the counter becomes non-zero, returns false.
    //! Deadline is an absolute timestamp in ClockMonotonic domain.
    ROC_NODISCARD bool timed_wait(nanoseconds_t deadline);

    //! Wait until the counter becomes non-zero, decrement it, and return.
    void wait();

    //! Increment counter and wake up blocked waits.
    void post();

private:
    Mutex mutex_;
    Cond cond_;
    unsigned counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEMAPHORE_H_
