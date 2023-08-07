/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/rate_limiter.h"

namespace roc {
namespace core {

RateLimiter::RateLimiter(nanoseconds_t period)
    : period_(Ticker::ticks_t(period))
    , pos_(0)
    , ticker_(Second / Nanosecond) {
    if (period <= 0) {
        roc_panic("rate limiter: expected positive period, got %ld", (long)period);
    }
}

bool RateLimiter::would_allow() {
    return ticker_.elapsed() >= pos_;
}

bool RateLimiter::allow() {
    const Ticker::ticks_t elapsed = ticker_.elapsed();
    if (elapsed >= pos_) {
        pos_ = (elapsed / period_ + 1) * period_;
        return true;
    } else {
        return false;
    }
}

} // namespace core
} // namespace roc
