/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/timer.h
//! @brief Thread-safe timer.

#ifndef ROC_CORE_TIMER_H_
#define ROC_CORE_TIMER_H_

#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Thread-safe timer.
class Timer : public NonCopyable<> {
public:
    Timer();

    //! Set timer deadline.
    //! All blocking wait_deadline() calls will unblock when deadline expires.
    //! Zero deadline means wake up immediately.
    //! Nagative deadline means never wake up, until deadline is changed again.
    void set_deadline(nanoseconds_t deadline);

    //! Wait until deadline expires.
    //! Deadline may be changed concurrently from other threads.
    void wait_deadline();

private:
    Mutex mutex_;
    Cond cond_;

    nanoseconds_t deadline_;
    nanoseconds_t next_wakeup_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_TIMER_H_
