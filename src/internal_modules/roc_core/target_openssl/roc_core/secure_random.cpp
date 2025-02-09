/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/secure_random.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"

#include <openssl/err.h>
#include <openssl/rand.h>

namespace roc {
namespace core {

bool secure_random(void* buf, size_t bufsz) {
    // RAND_priv_bytes() is not needed right now as we do not use these random numbers
    // privately. See also https://docs.openssl.org/3.0/man7/RAND/

    int ok = RAND_bytes((unsigned char*)buf, (int)bufsz);

    if (ok != 1) {
        unsigned long err;
        char err_str[256]; // minimum buf length is 256
        memset(err_str, 0, ROC_ARRAY_SIZE(err_str));
        while ((err = ERR_get_error()) != 0) {
            ERR_error_string_n(err, err_str, ROC_ARRAY_SIZE(err_str));
            roc_log(LogError, "secure random: OpenSSL RAND_bytes() failed: %s", err_str);
        }
        return false;
    }
    return true;
}

bool secure_random_range_32(uint32_t from, uint32_t to, uint32_t& dest) {
    // validation `to >= from` exists in the 64-bit version called below

    if (from == 0 && to == UINT32_MAX) {
        return secure_random(&dest, sizeof(dest));
    }

    uint64_t rand64;
    bool ok = secure_random_range_64(from, to, rand64);
    if (!ok) {
        return false;
    }
    dest = (uint32_t)rand64;
    return true;
}

bool secure_random_range_64(uint64_t from, uint64_t to, uint64_t& dest) {
    // same as fast_random_range() except it calls CSPRNG to get a uint64 value

    roc_panic_if_msg(from > to, "secure random: invalid range: from=%llu to=%llu",
                     (unsigned long long)from, (unsigned long long)to);

    // corner case check; needed to avoid a possible uint64_t overflow
    if (from == 0 && to == UINT64_MAX) {
        return secure_random(&dest, sizeof(dest));
    }

    const uint64_t range = to - from + 1;

    // Generate a mask with 1's from bit 0 to the most significant bit in `range`.
    // At each step, we double the count of leading 1's:
    //  0001.......
    //  00011......
    //  0001111....
    // Thanks to @rnovatorov for the hint.
    uint64_t mask = range;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;
    mask |= mask >> 16;
    mask |= mask >> 32;

    do {
        bool ok = secure_random(&dest, sizeof(dest));
        if (!ok) {
            return false;
        }
        dest &= mask;
    } while (dest >= range);

    dest += from;

    roc_panic_if_not(dest >= from);
    roc_panic_if_not(dest <= to);

    return true;
}

} // namespace core
} // namespace roc
