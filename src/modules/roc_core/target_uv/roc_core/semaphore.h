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
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Semaphore.
class Semaphore : public NonCopyable<> {
public:
    //! Initialize semaphore with the given initial counter value.
    explicit Semaphore(size_t counter);

    ~Semaphore();

    //! Increment semaphore counter.
    //! @remarks
    //!  Notifies threads that are blocked on pend().
    void post();

    //! Decrement semaphore counter.
    //! @remarks
    //!  Blocks until counter will be greather than zero.
    //!  The order of waking up blocked pends is not guaranteed.
    void pend();

    //! Try to decrement semaphore counter.
    //! @remarks
    //!  Decrements counter if it's greather than zero or returns immediately
    //!  otherwise.
    //! @returns
    //!  true if the counter was decremented.
    bool try_pend();

private:
    uv_mutex_t mutex_;
    uv_cond_t cond_;
    size_t counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEMAPHORE_H_
