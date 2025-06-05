/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/timer.h
//! @brief Thread-safe timer.

#ifndef ROC_CORE_TIMER_H_
#define ROC_CORE_TIMER_H_

#include "roc_core/atomic_bool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/semaphore.h"
#include "roc_core/seqlock.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Thread-safe timer.
class Timer : public NonCopyable<> {
public:
    Timer();

    //! Set timer deadline.
    //! Can be called concurrently, but only one concurrent call will succeed.
    //! Returns false if the call failed because of another concurrent call.
    //! Is lock-free if Semaphore::post() is so (which is true of modern plarforms).
    //! Current or future wait_deadline() call will unblock when deadline expires.
    //! Zero deadline means wake up immediately.
    //! Nagative deadline means never wake up, until deadline is changed again.
    bool try_set_deadline(nanoseconds_t deadline);

    //! Wait until deadline expires.
    //! Should be called from a single thread.
    //! Assumes that wait_deadline() calls are serialized.
    //! Deadline may be changed concurrently from other threads.
    void wait_deadline();

private:
    Semaphore sem_;
    AtomicBool sem_post_flag_;
    Seqlock<nanoseconds_t> deadline_;
    Seqlock<nanoseconds_t> next_wakeup_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_TIMER_H_
