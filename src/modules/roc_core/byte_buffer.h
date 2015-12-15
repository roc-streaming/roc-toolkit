/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/byte_buffer.h
//! @brief Byte buffer.

#ifndef ROC_CORE_BYTE_BUFFER_H_
#define ROC_CORE_BYTE_BUFFER_H_

#include "roc_core/buffer_traits.h"

namespace roc {
namespace core {

//! Byte buffer traits.
typedef BufferTraits<uint8_t> ByteBufferTraits;

//! Byte buffer interface.
typedef ByteBufferTraits::Interface IByteBuffer;

//! Byte buffer smart pointer.
typedef ByteBufferTraits::Ptr IByteBufferPtr;

//! Const byte buffer smart pointer.
typedef ByteBufferTraits::ConstPtr IByteBufferConstPtr;

//! Byte buffer slice.
typedef ByteBufferTraits::Slice IByteBufferSlice;

//! Const byte buffer slice.
typedef ByteBufferTraits::ConstSlice IByteBufferConstSlice;

//! Byte buffer composer interface.
typedef ByteBufferTraits::Composer IByteBufferComposer;

} // namespace core
} // namespace roc

#endif // ROC_CORE_BYTE_BUFFER_H_
