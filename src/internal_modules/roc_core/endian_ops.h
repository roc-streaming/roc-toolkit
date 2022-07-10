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

#if ROC_CPU_BIG_ENDIAN
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

#if ROC_CPU_BIG_ENDIAN
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
        return uint16_t((v >> 8) & 0xffu) | uint16_t((v & 0xffu) << 8);
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
        return ((v & 0x00000000ffffffffu) << 32) | ((v & 0xffffffff00000000u) >> 32)
            | ((v & 0x0000ffff0000ffffu) << 16) | ((v & 0xffff0000ffff0000u) >> 16)
            | ((v & 0x00ff00ff00ff00ffu) << 8) | ((v & 0xff00ff00ff00ff00u) >> 8);
    }

    static inline int64_t reverse_octets_(int64_t v) {
        return (int64_t)reverse_octets_((uint64_t)v);
    }

    template <class T> static inline T reverse_octets_(T v) {
        enum { NumOctets = sizeof(T) };

        union {
            T value;
            char octets[sizeof(T)];
        } u;

        u.value = v;

        switch (NumOctets) {
        case 0:
        case 1:
            break;

        case 2:
            std::swap(u.octets[0], u.octets[1]);
            break;

        case 3:
            std::swap(u.octets[0], u.octets[2]);
            break;

        case 4:
            std::swap(u.octets[0], u.octets[3]);
            std::swap(u.octets[1], u.octets[2]);
            break;

        case 5:
            std::swap(u.octets[0], u.octets[4]);
            std::swap(u.octets[1], u.octets[3]);
            break;

        case 6:
            std::swap(u.octets[0], u.octets[5]);
            std::swap(u.octets[1], u.octets[4]);
            std::swap(u.octets[2], u.octets[3]);
            break;

        case 7:
            std::swap(u.octets[0], u.octets[6]);
            std::swap(u.octets[1], u.octets[5]);
            std::swap(u.octets[2], u.octets[4]);
            break;

        case 8:
            std::swap(u.octets[0], u.octets[7]);
            std::swap(u.octets[1], u.octets[6]);
            std::swap(u.octets[2], u.octets[5]);
            std::swap(u.octets[3], u.octets[4]);
            break;

        default:
            for (size_t n = 0; n < NumOctets / 2; n++) {
                std::swap(u.octets[n], u.octets[NumOctets - n - 1]);
            }
            break;
        }

        return v;
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ENDIAN_OPS_H_
