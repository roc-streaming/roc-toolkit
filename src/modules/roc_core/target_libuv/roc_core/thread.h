/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libuv/roc_core/thread.h
//! @brief Thread.

#ifndef ROC_CORE_THREAD_H_
#define ROC_CORE_THREAD_H_

#include <uv.h>

#include "roc_core/atomic.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Base class for thread objects.
class Thread : public NonCopyable<Thread> {
public:
    //! Check if thread was started and can be joined.
    //! @returns
    //!  true if start() was called and join() was not called yet.
    bool joinable() const;

    //! Start thread.
    //! @remarks
    //!  Executes run() in new thread.
    bool start();

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
    static void thread_runner_(void* ptr);

    uv_thread_t thread_;

    int started_;
    Atomic joinable_;

    Mutex mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_THREAD_H_
