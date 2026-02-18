/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/rate_limiter.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

RateLimiter::RateLimiter(nanoseconds_t period, size_t burst)
    : period_((ticks_t)period)
    , burst_(burst)
    , ticker_(Second / Nanosecond) // 1 tick = 1 ns
    , token_expiration_(0)
    , token_count_(0) {
    roc_panic_if_msg(period <= 0, "rate limiter: period must be > 0");
    roc_panic_if_msg(burst <= 0, "rate limiter: burst must be > 0");
}

bool RateLimiter::would_allow() {
    const ticks_t elapsed = ticker_.elapsed();

    return elapsed >= token_expiration_ || token_count_ > 0;
}

bool RateLimiter::allow() {
    const ticks_t elapsed = ticker_.elapsed();

    if (elapsed >= token_expiration_) {
        token_expiration_ = (elapsed / period_ + 1) * period_;
        token_count_ = burst_;
    }

    if (token_count_ > 0) {
        token_count_--;
        return true;
    }

    return false;
}

} // namespace core
} // namespace roc
