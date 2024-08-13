/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/cond.h
//! @brief Condition variable.

#ifndef ROC_CORE_COND_H_
#define ROC_CORE_COND_H_

#include "roc_core/atomic.h"
#include "roc_core/attributes.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"

#include <errno.h>
#include <pthread.h>

namespace roc {
namespace core {

//! Condition variable.
class Cond : public NonCopyable<> {
public:
    //! Initialize.
    Cond(const Mutex& mutex);

    //! Destroy.
    ~Cond();

    //! Wait with timeout.
    //! @returns false if timeout expired.
    ROC_ATTR_NODISCARD bool timed_wait(nanoseconds_t timeout) const;

    //! Wait.
    void wait() const;

    //! Wake up one pending waits.
    void signal() const;

    //! Wake up all pending waits.
    void broadcast() const;

private:
    mutable pthread_cond_t cond_;
    mutable Atomic<int> guard_;

    pthread_mutex_t& mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_COND_H_
