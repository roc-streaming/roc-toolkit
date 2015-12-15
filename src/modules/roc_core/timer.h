/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/timer.h
//! @brief Timer object.

#ifndef ROC_CORE_TIMER_H_
#define ROC_CORE_TIMER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Timer object.
class Timer : public NonCopyable<> {
public:
    //! Initialize timer.
    //! @remarks
    //!  @p period_ms is timer tick duration in milliseconds.
    Timer(uint64_t period_ms)
        : period_(period_ms)
        , timestamp_(timestamp_ms()) {
    }

    //! Check if one or more next timer ticks occured.
    //! @returns
    //!  true when called first time after previous timer tick.
    bool expired() const {
        const uint64_t now = timestamp_ms();

        if ((now - timestamp_) < period_) {
            return false;
        }

        timestamp_ += (now - timestamp_) / period_ * period_;
        return true;
    }

private:
    const uint64_t period_;
    mutable uint64_t timestamp_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_TIMER_H_
