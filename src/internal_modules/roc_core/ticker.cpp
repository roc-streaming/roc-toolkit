/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/ticker.h"

namespace roc {
namespace core {

Ticker::Ticker(ticks_t freq)
    : ratio_(double(freq) / Second)
    , start_(0)
    , started_(false) {
}

void Ticker::start() {
    if (started_) {
        roc_panic("ticker: can't start ticker twice");
    }
    start_ = timestamp(ClockMonotonic);
    started_ = true;
}

Ticker::ticks_t Ticker::elapsed() {
    if (!started_) {
        start();
        return 0;
    } else {
        return ticks_t(double(timestamp(ClockMonotonic) - start_) * ratio_);
    }
}

void Ticker::wait(ticks_t ticks) {
    if (!started_) {
        start();
    }
    sleep_until(ClockMonotonic, start_ + nanoseconds_t(ticks / ratio_));
}

} // namespace core
} // namespace roc
