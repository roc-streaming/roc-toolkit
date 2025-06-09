/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_windows/roc_core/mutex.h
//! @brief Mutex.

#ifndef ROC_CORE_MUTEX_H_
#define ROC_CORE_MUTEX_H_

#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_lock.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

class Cond;

//! Mutex.
class Mutex : public NonCopyable<> {
public:
    //! RAII lock.
    typedef ScopedLock<Mutex> Lock;

    //! Initialize mutex.
    Mutex();

    //! Destroy mutex.
    ~Mutex();

    //! Try to lock the mutex.
    ROC_NODISCARD inline bool try_lock() const {
        return TryEnterCriticalSection(&mutex_) != 0;
    }

    //! Lock mutex.
    inline void lock() const {
        EnterCriticalSection(&mutex_);
    }

    //! Unlock mutex.
    inline void unlock() const {
        LeaveCriticalSection(&mutex_);
    }

private:
    friend class Cond;

    mutable CRITICAL_SECTION mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MUTEX_H_
