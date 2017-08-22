/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_darwin/roc_core/time.h
//! @brief Time functions.

#ifndef ROC_CORE_TIME_H_
#define ROC_CORE_TIME_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Get current timestamp in milliseconds.
uint64_t timestamp_ms();

//! Sleep until specified absolute time point has been reached.
//! @remarks
//!  @p timestamp specifies time point in milleseconds.
void sleep_until_ms(uint64_t timestamp);

//! Sleep specified amount of time.
//! @remarks
//!  @p timestamp specifies number of milleseconds to sleep.
void sleep_for_ms(uint64_t time);

} // namespace core
} // namespace roc

#endif // ROC_CORE_TIME_H_
