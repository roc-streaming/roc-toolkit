/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/print_buffer.h
//! @brief Print buffer to console.

#ifndef ROC_CORE_PRINT_BUFFER_H_
#define ROC_CORE_PRINT_BUFFER_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Print buffer.
void print_buffer(const uint8_t* data, size_t size);

//! Print buffer.
void print_buffer(const uint16_t* data, size_t size);

//! Print buffer.
void print_buffer(const uint32_t* data, size_t size);

//! Print buffer.
void print_buffer(const uint64_t* data, size_t size);

//! Print buffer.
void print_buffer(const int8_t* data, size_t size);

//! Print buffer.
void print_buffer(const int16_t* data, size_t size);

//! Print buffer.
void print_buffer(const int32_t* data, size_t size);

//! Print buffer.
void print_buffer(const int64_t* data, size_t size);

//! Print floats buffer.
void print_buffer(const float* data, size_t size);

//! Print floats buffer.
void print_buffer(const double* data, size_t size);

//! Print a slice of a buffer.
void print_buffer_slice(const uint8_t* inner,
                        size_t inner_size,
                        const uint8_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const uint16_t* inner,
                        size_t inner_size,
                        const uint16_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const uint32_t* inner,
                        size_t inner_size,
                        const uint32_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const uint64_t* inner,
                        size_t inner_size,
                        const uint64_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const int8_t* inner,
                        size_t inner_size,
                        const int8_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const int16_t* inner,
                        size_t inner_size,
                        const int16_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const int32_t* inner,
                        size_t inner_size,
                        const int32_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const int64_t* inner,
                        size_t inner_size,
                        const int64_t* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const float* inner,
                        size_t inner_size,
                        const float* outer,
                        size_t outer_size);

//! Print a slice of a buffer.
void print_buffer_slice(const double* inner,
                        size_t inner_size,
                        const double* outer,
                        size_t outer_size);

} // namespace core
} // namespace roc

#endif // ROC_CORE_PRINT_BUFFER_H_
