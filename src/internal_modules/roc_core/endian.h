/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/endian.h
//! @brief Endian conversion functions.

#ifndef ROC_CORE_ENDIAN_H_
#define ROC_CORE_ENDIAN_H_

// This file provides explicitly-sized conveniently named short-hands
// for endian conversion functions.

#include "roc_core/endian_ops.h"

namespace roc {
namespace core {

//! Network to host byte order (unsigned 16-bit).
inline uint16_t ntoh16u(uint16_t v) {
    return EndianOps::swap_native_be(v);
}

//! Network to host byte order (signed 16-bit).
inline int16_t ntoh16s(int16_t v) {
    return EndianOps::swap_native_be(v);
}

//! Network to host byte order (unsigned 32-bit).
inline uint32_t ntoh32u(uint32_t v) {
    return EndianOps::swap_native_be(v);
}

//! Network to host byte order (signed 32-bit).
inline int32_t ntoh32s(int32_t v) {
    return EndianOps::swap_native_be(v);
}

//! Network to host byte order (unsigned 64-bit).
inline uint64_t ntoh64u(uint64_t v) {
    return EndianOps::swap_native_be(v);
}

//! Network to host byte order (signed 64-bit).
inline int64_t ntoh64s(int64_t v) {
    return EndianOps::swap_native_be(v);
}

//! Host to network byte order (unsigned 16-bit).
inline uint16_t hton16u(uint16_t v) {
    return EndianOps::swap_native_be(v);
}

//! Host to network byte order (signed 16-bit).
inline int16_t hton16s(int16_t v) {
    return EndianOps::swap_native_be(v);
}

//! Host to network byte order (unsigned 32-bit).
inline uint32_t hton32u(uint32_t v) {
    return EndianOps::swap_native_be(v);
}

//! Host to network byte order (signed 32-bit).
inline int32_t hton32s(int32_t v) {
    return EndianOps::swap_native_be(v);
}

//! Host to network byte order (unsigned 64-bit).
inline uint64_t hton64u(uint64_t v) {
    return EndianOps::swap_native_be(v);
}

//! Host to network byte order (signed 64-bit).
inline int64_t hton64s(int64_t v) {
    return EndianOps::swap_native_be(v);
}

} // namespace core
} // namespace roc

#endif // ROC_CORE_ENDIAN_H_
