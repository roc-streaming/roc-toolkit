/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/lock.h
//! @brief RAII mutex lock.

#ifndef ROC_CORE_LOCK_H_
#define ROC_CORE_LOCK_H_

#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! RAII mutex lock.
class Lock : NonCopyable<> {
public:
    //! Lock.
    explicit Lock(const Mutex& mutex)
        : mutex_(mutex) {
        mutex_.lock();
    }

    //! Unlock.
    ~Lock() {
        mutex_.unlock();
    }

private:
    const Mutex& mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LOCK_H_
