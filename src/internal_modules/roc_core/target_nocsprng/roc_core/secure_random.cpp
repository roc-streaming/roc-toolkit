/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/secure_random.h"
#include "roc_core/fast_random.h"

namespace roc {
namespace core {

bool secure_random(void* buf, size_t bufsz) {
    unsigned char* ubuf = (unsigned char*)buf;

    size_t i = 0;
    while (i < bufsz) {
        union {
            uint32_t number;
            unsigned char bytes[4];
        } rand;
        rand.number = fast_random_32();

        // clang-format off
        ubuf[i] = rand.bytes[0]; i++; if (i >= bufsz) { break; }
        ubuf[i] = rand.bytes[1]; i++; if (i >= bufsz) { break; }
        ubuf[i] = rand.bytes[2]; i++; if (i >= bufsz) { break; }
        ubuf[i] = rand.bytes[3]; i++;
        // clang-format on
    }
    return true;
}

bool secure_random_range_32(uint32_t from, uint32_t to, uint32_t& dest) {
    dest = (uint32_t)fast_random_range(from, to);
    return true;
}

bool secure_random_range_64(uint64_t from, uint64_t to, uint64_t& dest) {
    dest = fast_random_range(from, to);
    return true;
}

} // namespace core
} // namespace roc
