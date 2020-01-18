/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/format_tid.h
//! @brief Retrieve the current thread id.

#ifndef ROC_CORE_FORMAT_TID_H_
#define ROC_CORE_FORMAT_TID_H_

namespace roc {
namespace core {

//! Retrieve and format current time.
//!
//! @returns
//!  if obtaining thread id was successful
//!
//! @note
//!  This function should not log anything because it is used
//!  in the logger implementation.
bool format_tid(char* buf, size_t bufsz);

} // namespace core
} // namespace roc

#endif // ROC_CORE_FORMAT_TID_H_
