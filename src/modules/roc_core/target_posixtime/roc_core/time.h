/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posixtime/roc_core/time.h
//! @brief Time functions.

#ifndef ROC_CORE_TIME_H_
#define ROC_CORE_TIME_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Nanoseconds.
typedef uint64_t nanoseconds_t;

//! Get current timestamp in nanoseconds.
nanoseconds_t timestamp();

//! Sleep until the specified absolute time point has been reached.
//! @remarks
//!  @p timestamp specifies time point in nanoseconds.
void sleep_until(nanoseconds_t timestamp);

//! Sleep specified amount of time.
//! @remarks
//!  @p timestamp specifies number of nanoseconds to sleep.
void sleep_for(nanoseconds_t time);

} // namespace core
} // namespace roc

#endif // ROC_CORE_TIME_H_
