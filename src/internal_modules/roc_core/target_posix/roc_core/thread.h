/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/thread.h
//! @brief Thread.

#ifndef ROC_CORE_THREAD_H_
#define ROC_CORE_THREAD_H_

#include "roc_core/atomic.h"
#include "roc_core/attributes.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

#include <pthread.h>

namespace roc {
namespace core {

//! Base class for thread objects.
class Thread : public NonCopyable<Thread> {
public:
    //! Get numeric identifier of current process.
    static uint64_t get_pid();

    //! Get numeric identifier of current thread.
    static uint64_t get_tid();

    //! Raise current thread priority to realtime.
    ROC_ATTR_NODISCARD static bool enable_realtime();

    //! Check if thread was started and can be joined.
    //! @returns
    //!  true if start() was called and join() was not called yet.
    bool is_joinable() const;

    //! Start thread.
    //! @remarks
    //!  Executes run() in new thread.
    ROC_ATTR_NODISCARD bool start();

    //! Join thread.
    //! @remarks
    //!  Blocks until run() returns and thread terminates.
    void join();

protected:
    virtual ~Thread();

    Thread();

    //! Method to be executed in thread.
    virtual void run() = 0;

private:
    static void* thread_runner_(void* ptr);

    pthread_t thread_;

    int started_;
    Atomic<int> joinable_;

    Mutex mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_THREAD_H_
