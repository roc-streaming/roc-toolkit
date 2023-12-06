/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/hashsum.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

hashsum_t hashsum_int(int16_t x) {
    return hashsum_int((uint16_t)x);
}

hashsum_t hashsum_int(uint16_t x) {
    // https://github.com/skeeto/hash-prospector
    x = ((x >> 8) ^ x) * 0x88b5;
    x = ((x >> 7) ^ x) * 0xdb2d;
    x = ((x >> 9) ^ x);

    if (sizeof(hashsum_t) == sizeof(uint64_t)) {
        return (uint64_t(x) << 48) | (uint64_t(x) << 32) | (uint64_t(x) << 16)
            | uint64_t(x);
    } else {
        return (uint32_t(x) << 16) | uint64_t(x);
    }
}

hashsum_t hashsum_int(int32_t x) {
    return hashsum_int((uint32_t)x);
}

hashsum_t hashsum_int(uint32_t x) {
    // https://github.com/skeeto/hash-prospector
    x = ((x >> 16) ^ x) * 0x7feb352d;
    x = ((x >> 15) ^ x) * 0x846ca68b;
    x = ((x >> 16) ^ x);

    if (sizeof(hashsum_t) == sizeof(uint64_t)) {
        return (uint64_t(x) << 32) | uint64_t(x);
    } else {
        return uint32_t(x);
    }
}

hashsum_t hashsum_int(int64_t x) {
    return hashsum_int((uint64_t)x);
}

hashsum_t hashsum_int(uint64_t x) {
    // https://stackoverflow.com/a/12996028/3169754
    x = (x ^ (x >> 30)) * uint64_t(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * uint64_t(0x94d049bb133111eb);
    x = (x ^ (x >> 31));

    return hashsum_t(x);
}

hashsum_t hashsum_str(const char* str) {
    roc_panic_if(!str);

    hashsum_t h = 0;
    hashsum_add(h, str, strlen(str));

    return h;
}

hashsum_t hashsum_mem(const void* data, size_t size) {
    roc_panic_if(!data);

    hashsum_t h = 0;
    hashsum_add(h, data, size);

    return h;
}

void hashsum_add(hashsum_t& h, const void* data, size_t size) {
    roc_panic_if(!data);

    // DJB2
    // https://stackoverflow.com/a/2624218/3169754
    if (h == 0) {
        h = 5381;
    }
    for (size_t n = 0; n < size; n++) {
        h = ((h << 5) + h) + ((const uint8_t*)data)[n];
    }
}

} // namespace core
} // namespace roc
