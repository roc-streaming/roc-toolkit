/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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
#include "roc_core/ticker.h"

namespace roc {
namespace core {

//! Rate limiter.
class RateLimiter : public NonCopyable<> {
public:
    //! Initialize rate limiter.
    //! @remarks
    //!  @p period_ns is tick duration in nanoseconds.
    RateLimiter(nanoseconds_t period_ns)
        : period_(period_ns)
        , pos_(0)
        , ticker_(1000000000) {
    }

    //! Check whether an event is allowed to occur now.
    bool allow() {
        const uint64_t elapsed = ticker_.elapsed();
        if (elapsed >= pos_) {
            pos_ = (elapsed / period_ + 1) * period_;
            return true;
        } else {
            return false;
        }
    }

private:
    const nanoseconds_t period_;
    nanoseconds_t pos_;
    Ticker ticker_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_RATE_LIMITER_H_
