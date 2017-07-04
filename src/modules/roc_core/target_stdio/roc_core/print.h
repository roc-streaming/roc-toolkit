/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_stdio/roc_core/print.h
//! @brief Print buffer to stdout.

#ifndef ROC_CORE_PRINT_H_
#define ROC_CORE_PRINT_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Print bytes to stderr.
void print_bytes(const uint8_t* data, size_t size);

//! Print floats to stderr.
void print_floats(const float* data, size_t size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PRINT_H_
