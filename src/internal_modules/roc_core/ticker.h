/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ticker.h
//! @brief Ticker.

#ifndef ROC_CORE_TICKER_H_
#define ROC_CORE_TICKER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Ticker.
class Ticker : public NonCopyable<> {
public:
    //! Number of ticks.
    typedef uint64_t Ticks;

    //! Initialize.
    //! @remarks
    //!  @p freq defines the number of ticks per second.
    explicit Ticker(Ticks freq)
        : ratio_(double(freq) / Second)
        , start_(0)
        , started_(false) {
    }

    //! Start ticker.
    void start() {
        if (started_) {
            roc_panic("ticker: can't start ticker twice");
        }
        start_ = timestamp();
        started_ = true;
    }

    //! Returns number of ticks elapsed since start.
    //! If ticker is not started yet, it is started automatically.
    Ticks elapsed() {
        if (!started_) {
            start();
            return 0;
        } else {
            return Ticks(double(timestamp() - start_) * ratio_);
        }
    }

    //! Wait until the given number of ticks elapses since start.
    //! If ticker is not started yet, it is started automatically.
    void wait(Ticks ticks) {
        if (!started_) {
            start();
        }
        sleep_until(start_ + nanoseconds_t(ticks / ratio_));
    }

private:
    const double ratio_;
    nanoseconds_t start_;
    bool started_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_TICKER_H_
