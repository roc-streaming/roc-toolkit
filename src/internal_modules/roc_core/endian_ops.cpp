/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/endian_ops.h"

namespace roc {
namespace core {

uint8_t EndianOps::reverse_octets_(uint8_t v) {
    return v;
}

int8_t EndianOps::reverse_octets_(int8_t v) {
    return v;
}

uint16_t EndianOps::reverse_octets_(uint16_t v) {
    return uint16_t(uint16_t(v >> 8) & 0xffu) | uint16_t((v & 0xffu) << 8);
}

int16_t EndianOps::reverse_octets_(int16_t v) {
    return (int16_t)reverse_octets_((uint16_t)v);
}

uint32_t EndianOps::reverse_octets_(uint32_t v) {
    return (((v & 0xff000000u) >> 24) | ((v & 0x00ff0000u) >> 8)
            | ((v & 0x0000ff00u) << 8) | ((v & 0x000000ffu) << 24));
}

int32_t EndianOps::reverse_octets_(int32_t v) {
    return (int32_t)reverse_octets_((uint32_t)v);
}

uint64_t EndianOps::reverse_octets_(uint64_t v) {
    return ((v & 0xff00000000000000ull) >> 56) | ((v & 0x00ff000000000000ull) >> 40)
        | ((v & 0x0000ff0000000000ull) >> 24) | ((v & 0x000000ff00000000ull) >> 8)
        | ((v & 0x00000000ff000000ull) << 8) | ((v & 0x0000000000ff0000ull) << 24)
        | ((v & 0x000000000000ff00ull) << 40) | ((v & 0x00000000000000ffull) << 56);
}

int64_t EndianOps::reverse_octets_(int64_t v) {
    return (int64_t)reverse_octets_((uint64_t)v);
}

float EndianOps::reverse_octets_(float v) {
    union {
        float f;
        uint32_t i;
    } u;

    u.f = v;
    u.i = reverse_octets_(u.i);

    return u.f;
}

double EndianOps::reverse_octets_(double v) {
    union {
        double f;
        uint64_t i;
    } u;

    u.f = v;
    u.i = reverse_octets_(u.i);

    return u.f;
}

} // namespace core
} // namespace roc
