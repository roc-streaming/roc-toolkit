/*
 * Copyright (c) 2015 Roc authors
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

//! Print bytes.
void print_memory(const uint8_t* data, size_t size);

//! Print floats.
void print_memory(const float* data, size_t size);

//! Print byte slice.
void print_slice(const uint8_t* inner,
                 size_t inner_size,
                 const uint8_t* outer,
                 size_t outer_size);

//! Print float slice.
void print_slice(const float* inner,
                 size_t inner_size,
                 const float* outer,
                 size_t outer_size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PRINT_H_
