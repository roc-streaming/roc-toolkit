/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/parse_units.h
//! @brief Parse units like duration, size, etc.

#ifndef ROC_CORE_PARSE_UNITS_H_
#define ROC_CORE_PARSE_UNITS_H_

#include "roc_core/attributes.h"
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
ROC_ATTR_NODISCARD bool parse_duration(const char* string, nanoseconds_t& result);

//! Parse size from string.
//!
//! @remarks
//!  The input string should be in one of the following forms:
//!   - "<number>"
//!   - "<number>K"
//!   - "<number>M"
//!   - "<number>G"
//!
//! @returns
//!  false if string can't be parsed.
ROC_ATTR_NODISCARD bool parse_size(const char* string, size_t& result);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PARSE_UNITS_H_
