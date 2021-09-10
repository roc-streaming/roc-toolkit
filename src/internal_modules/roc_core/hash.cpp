/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/hash.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

hash_t hash_int(int32_t x) {
    return hash_int((uint32_t)x);
}

hash_t hash_int(uint32_t x) {
    // https://stackoverflow.com/a/12996028/3169754
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;

    if (sizeof(hash_t) == sizeof(uint64_t)) {
        return (uint64_t(x) << 32) | x;
    } else {
        return x;
    }
}

hash_t hash_int(int64_t x) {
    return hash_int((uint64_t)x);
}

hash_t hash_int(uint64_t x) {
    // https://stackoverflow.com/a/12996028/3169754
    x = (x ^ (x >> 30)) * uint64_t(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * uint64_t(0x94d049bb133111eb);
    x = x ^ (x >> 31);

    return hash_t(x);
}

hash_t hash_str(const char* str) {
    roc_panic_if(!str);

    return hash_mem(str, strlen(str));
}

hash_t hash_mem(const void* data, size_t size) {
    roc_panic_if(data == NULL || size == 0);

    // https://stackoverflow.com/a/2624218/3169754
    hash_t h = 5381;
    for (size_t n = 0; n < size; n++) {
        h = ((h << 5) + h) + ((const uint8_t*)data)[n];
    }

    return h;
}

} // namespace core
} // namespace roc
