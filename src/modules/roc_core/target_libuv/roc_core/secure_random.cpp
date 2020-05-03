/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <uv.h>

#include "roc_core/fast_random.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/secure_random.h"

namespace roc {
namespace core {

// The implementation is based on "Debiased Modulo (Once) — Java's Method" algorithm
// from https://www.pcg-random.org/posts/bounded-rands.html
//
// We use uv_random() without an event loop, which is thread-safe.
#if UV_VERSION_MAJOR > 1 || (UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR >= 33)
bool secure_random(uint32_t from, uint32_t to, uint32_t& result) {
    roc_panic_if_not(from <= to);

    const uint64_t range = uint64_t(to) - from + 1;

    uint64_t x;
    uint64_t r;

    do {
        uint32_t val = 0;
        if (int err = uv_random(NULL, NULL, &val, sizeof(val), 0, NULL)) {
            roc_log(LogError, "secure random : uv_random(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
            return false;
        }
        x = uint64_t(val);
        r = x % range;
    } while (x - r > (-range));

    result = from + (uint32_t)r;

    roc_panic_if_not(result >= from);
    roc_panic_if_not(result <= to);

    return true;
}
#else  // UV_VERSION_MAJOR < 1 || (UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR < 33)
bool secure_random(uint32_t from, uint32_t to, uint32_t& result) {
    result = fast_random(from, to);
    return true;
}
#endif // UV_VERSION_MAJOR > 1 || (UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR >= 33)

} // namespace core
} // namespace roc
