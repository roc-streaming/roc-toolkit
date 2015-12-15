/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_stdio/roc_core/print_buffer.h
//! @brief Print buffer to stdout.

#ifndef ROC_CORE_PRINT_BUFFER_H_
#define ROC_CORE_PRINT_BUFFER_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Print byte buffer to stdout.
void print_buffer(const uint8_t* data, size_t size, size_t max_size);

//! Print float buffer to stdout.
void print_buffer(const float* data, size_t size, size_t max_size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PRINT_BUFFER_H_
