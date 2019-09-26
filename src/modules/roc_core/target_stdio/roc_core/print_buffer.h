/*
 * Copyright (c) 2015 Roc authors
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

//! Print bytes buffer.
void print_buffer(const uint8_t* data, size_t size);

//! Print floats buffer.
void print_buffer(const float* data, size_t size);

//! Print a slice of bytes buffer.
void print_buffer_slice(const uint8_t* inner,
                        size_t inner_size,
                        const uint8_t* outer,
                        size_t outer_size);

//! Print a slice of floats buffer.
void print_buffer_slice(const float* inner,
                        size_t inner_size,
                        const float* outer,
                        size_t outer_size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PRINT_BUFFER_H_
