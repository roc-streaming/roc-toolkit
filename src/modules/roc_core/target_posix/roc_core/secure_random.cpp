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

// Based on uv_random from libuv and arc4random_uniform() from OpenBSD.
#if UV_VERSION_MAJOR > 1 || (UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR >= 33)
bool secure_random(uint32_t from, uint32_t to, uint32_t& result) {
    roc_panic_if_not(from <= to);

    const uint64_t upper = (uint64_t)to - from + 1;
    const uint64_t min = -upper % upper;
    uint64_t val;

    for (;;) {
        if (int err = uv_random(NULL, NULL, &val, sizeof(val), 0, NULL)) {
            roc_log(LogError, "secure random : uv_random(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
            return false;
        }
        if (val >= min) {
            break;
        }
    }

    const uint32_t ret = from + (uint32_t)(val % upper);

    roc_panic_if_not(ret >= from);
    roc_panic_if_not(ret <= to);

    result = ret;
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
