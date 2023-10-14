/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/endian_ops.h
//! @brief Endian operations.

#ifndef ROC_CORE_ENDIAN_OPS_H_
#define ROC_CORE_ENDIAN_OPS_H_

#include "roc_core/cpu_traits.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Endian operations.
class EndianOps {
public:
    //! Swap between endians.
    template <class T> static inline T swap_endian(T v) {
        return reverse_octets_(v);
    }

#if ROC_CPU_ENDIAN == ROC_CPU_BE
    //! Swap between native endian and big endian.
    template <class T> static inline T swap_native_be(T v) {
        return v;
    }
#else
    //! Swap between native endian and big endian.
    template <class T> static inline T swap_native_be(T v) {
        return reverse_octets_(v);
    }
#endif

#if ROC_CPU_ENDIAN == ROC_CPU_BE
    //! Swap between native endian and little endian.
    template <class T> static inline T swap_native_le(T v) {
        return reverse_octets_(v);
    }
#else
    //! Swap between native endian and little endian.
    template <class T> static inline T swap_native_le(T v) {
        return v;
    }
#endif

private:
    static uint8_t reverse_octets_(uint8_t v);
    static int8_t reverse_octets_(int8_t v);
    static uint16_t reverse_octets_(uint16_t v);
    static int16_t reverse_octets_(int16_t v);
    static uint32_t reverse_octets_(uint32_t v);
    static int32_t reverse_octets_(int32_t v);
    static uint64_t reverse_octets_(uint64_t v);
    static int64_t reverse_octets_(int64_t v);
    static float reverse_octets_(float v);
    static double reverse_octets_(double v);
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ENDIAN_OPS_H_
