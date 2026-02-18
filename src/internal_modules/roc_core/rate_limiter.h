/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/rate_limiter.h
//! @brief Rate limiter.

#ifndef ROC_CORE_RATE_LIMITER_H_
#define ROC_CORE_RATE_LIMITER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_core/ticker.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Rate limiter.
class RateLimiter : public NonCopyable<> {
public:
    //! Initialize rate limiter.
    //! @remarks
    //!  @p period is duration of one tick, in nanoseconds.
    //!  @p burst is how much events is allowed per one tick.
    RateLimiter(nanoseconds_t period, size_t burst);

    //! Check whether allow() would succeed.
    bool would_allow();

    //! Check whether an event is allowed to occur now, and if yes, mark it as occurred.
    bool allow();

private:
    const ticks_t period_;
    const size_t burst_;

    Ticker ticker_;

    ticks_t token_expiration_;
    size_t token_count_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_RATE_LIMITER_H_
