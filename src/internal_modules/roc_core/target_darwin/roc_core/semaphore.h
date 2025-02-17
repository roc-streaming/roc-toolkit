/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_darwin/roc_core/semaphore.h
//! @brief Semaphore.

#ifndef ROC_CORE_SEMAPHORE_H_
#define ROC_CORE_SEMAPHORE_H_

#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"

#include <mach/semaphore.h>

namespace roc {
namespace core {

//! Semaphore.
class Semaphore : public NonCopyable<> {
public:
    //! Initialize semaphore with given counter.
    explicit Semaphore(unsigned counter = 0);

    ~Semaphore();

    //! Wait until the counter becomes non-zero, decrement it, and return true.
    //! If deadline expires before the counter becomes non-zero, returns false.
    //! Deadline should be in the same time domain as core::timestamp().
    ROC_ATTR_NODISCARD bool timed_wait(nanoseconds_t deadline);

    //! Wait until the counter becomes non-zero, decrement it, and return.
    void wait();

    //! Increment counter and wake up blocked waits.
    //! This method is lock-free.
    void post();

private:
    semaphore_t sem_id_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEMAPHORE_H_
