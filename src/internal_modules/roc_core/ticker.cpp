/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/ticker.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

namespace {

ticks_t ns_2_ticks(nanoseconds_t ns, double ticks_per_second) {
    return (ticks_t)round(double(ns) / Second * ticks_per_second);
}

nanoseconds_t ticks_2_ns(ticks_t ticks, double ticks_per_second) {
    return (nanoseconds_t)round(double(ticks) / ticks_per_second * Second);
}

} // namespace

Ticker::Ticker(ticks_t ticks_per_second)
    : ticks_per_second_((double)ticks_per_second)
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

ticks_t Ticker::elapsed() {
    if (!started_) {
        start();
        return 0;
    } else {
        return ns_2_ticks(timestamp(ClockMonotonic) - start_, ticks_per_second_);
    }
}

void Ticker::wait(ticks_t ticks) {
    if (!started_) {
        start();
    }
    sleep_until(ClockMonotonic, start_ + ticks_2_ns(ticks, ticks_per_second_));
}

} // namespace core
} // namespace roc
