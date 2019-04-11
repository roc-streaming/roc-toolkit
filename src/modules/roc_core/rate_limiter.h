/*
 * Copyright (c) 2017 Roc authors
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
    //!  @p period is tick duration in nanoseconds.
    explicit RateLimiter(nanoseconds_t period)
        : period_(Ticker::Ticks(period))
        , pos_(0)
        , ticker_(Second / Nanosecond) {
        if (period <= 0) {
            roc_panic("rate limiter: expected positive period, got %ld", (long)period);
        }
    }

    //! Check whether an event is allowed to occur now.
    bool allow() {
        const Ticker::Ticks elapsed = ticker_.elapsed();
        if (elapsed >= pos_) {
            pos_ = (elapsed / period_ + 1) * period_;
            return true;
        } else {
            return false;
        }
    }

private:
    const Ticker::Ticks period_;
    Ticker::Ticks pos_;
    Ticker ticker_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_RATE_LIMITER_H_
