/*
 * Copyright (c) 2017 Roc Streaming authors
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
#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Number of ticks.
typedef uint64_t ticks_t;

//! Ticker.
class Ticker : public NonCopyable<> {
public:
    //! Initialize.
    explicit Ticker(ticks_t ticks_per_second);

    //! Start ticker.
    void start();

    //! Returns number of ticks elapsed since start.
    //! If ticker is not started yet, it is started automatically.
    ticks_t elapsed();

    //! Wait until the given number of ticks elapses since start.
    //! If ticker is not started yet, it is started automatically.
    void wait(ticks_t ticks);

private:
    const double ticks_per_second_;
    nanoseconds_t start_;
    bool started_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_TICKER_H_
