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
    static inline uint8_t reverse_octets_(uint8_t v) {
        return v;
    }

    static inline int8_t reverse_octets_(int8_t v) {
        return v;
    }

    static inline uint16_t reverse_octets_(uint16_t v) {
        return uint16_t(uint16_t(v >> 8) & 0xffu) | uint16_t((v & 0xffu) << 8);
    }

    static inline int16_t reverse_octets_(int16_t v) {
        return (int16_t)reverse_octets_((uint16_t)v);
    }

    static inline uint32_t reverse_octets_(uint32_t v) {
        return (((v & 0xff000000u) >> 24) | ((v & 0x00ff0000u) >> 8)
                | ((v & 0x0000ff00u) << 8) | ((v & 0x000000ffu) << 24));
    }

    static inline int32_t reverse_octets_(int32_t v) {
        return (int32_t)reverse_octets_((uint32_t)v);
    }

    static inline uint64_t reverse_octets_(uint64_t v) {
        return ((v & 0xff00000000000000ull) >> 56) | ((v & 0x00ff000000000000ull) >> 40)
            | ((v & 0x0000ff0000000000ull) >> 24) | ((v & 0x000000ff00000000ull) >> 8)
            | ((v & 0x00000000ff000000ull) << 8) | ((v & 0x0000000000ff0000ull) << 24)
            | ((v & 0x000000000000ff00ull) << 40) | ((v & 0x00000000000000ffull) << 56);
    }

    static inline int64_t reverse_octets_(int64_t v) {
        return (int64_t)reverse_octets_((uint64_t)v);
    }

    static inline float reverse_octets_(float v) {
        union {
            float f;
            uint32_t i;
        } u;

        u.f = v;
        u.i = reverse_octets_(u.i);

        return u.f;
    }

    static inline double reverse_octets_(double v) {
        union {
            double f;
            uint64_t i;
        } u;

        u.f = v;
        u.i = reverse_octets_(u.i);

        return u.f;
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ENDIAN_OPS_H_
