/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/uuid.h"
#include "roc_core/fast_random.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

bool uuid_generare(char* buf, size_t buf_sz) {
    if (!buf) {
        roc_panic("uuid: buffer is null");
    }
    if (buf_sz < UuidLen + 1) {
        roc_panic("uuid: buffer is null");
    }

    const char* hex_chars = "0123456789abcdef";

    for (size_t i = 0; i < UuidLen; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            buf[i] = '-';
        } else {
            buf[i] = hex_chars[fast_random_range(0, 15)];
        }
    }

    buf[UuidLen] = '\0';

    return true;
}

} // namespace core
} // namespace roc
