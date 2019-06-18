/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/format_time.h
//! @brief Retrieve and format current time.

#ifndef ROC_CORE_FORMAT_TIME_H_
#define ROC_CORE_FORMAT_TIME_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Retrieve and format current time.
//
//! @remarks
//!  The time is printed in the format "13:10:05.123".
//!
//! @returns
//!  false if an error occured or buffer is too small.
//!
//! @note
//!  This function should not log anything because it is used
//!  in the logger implementation.
bool format_time(char* buf, size_t bufsz);

} // namespace core
} // namespace roc

#endif // ROC_CORE_FORMAT_TIME_H_
