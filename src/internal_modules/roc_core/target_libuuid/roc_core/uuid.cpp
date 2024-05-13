/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/uuid.h"

#include "roc_core/panic.h"

#include <uuid/uuid.h>

namespace roc {
namespace core {

// Uses libuuid to generate the uuid.
bool uuid_generare(char* buf, size_t buf_sz) {
    if (!buf) {
        roc_panic("uuid: buffer is null");
    }
    if (buf_sz < UuidLen + 1) {
        roc_panic("uuid: buffer too small");
    }

    uuid_t u;

    uuid_generate(u);
    uuid_unparse_lower(u, buf);

    return true;
}

} // namespace core
} // namespace roc
