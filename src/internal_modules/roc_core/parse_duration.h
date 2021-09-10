/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/parse_duration.h
//! @brief Parse duration.

#ifndef ROC_CORE_PARSE_DURATION_H_
#define ROC_CORE_PARSE_DURATION_H_

#include "roc_core/time.h"

namespace roc {
namespace core {

//! Parse duration from string.
//!
//! @remarks
//!  The input string should be in one of the following forms:
//!   - "<number>ns"
//!   - "<number>us"
//!   - "<number>ms"
//!   - "<number>s"
//!   - "<number>m"
//!   - "<number>h"
//!
//! @returns
//!  false if string can't be parsed.
bool parse_duration(const char* string, nanoseconds_t& result);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PARSE_DURATION_H_
