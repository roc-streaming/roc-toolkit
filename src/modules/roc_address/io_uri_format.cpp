/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_address/pct.h"
#include "roc_core/panic.h"
#include "roc_core/string_utils.h"

namespace roc {
namespace address {

bool format_io_uri(const IoURI& u, char* buf, size_t buf_size) {
    roc_panic_if(buf == NULL);

    if (buf_size == 0) {
        return false;
    }

    buf[0] = '\0';

    if (*u.scheme) {
        if (!core::append_str(buf, buf_size, u.scheme)) {
            return false;
        }

        if (u.is_file()) {
            if (!core::append_str(buf, buf_size, ":")) {
                return false;
            }
        } else {
            if (!core::append_str(buf, buf_size, "://")) {
                return false;
            }
        }
    }

    if (*u.path) {
        const size_t pos = strlen(buf);

        if (pct_encode(buf + pos, buf_size - pos, u.path, strlen(u.path), PctNonPath)
            == -1) {
            return false;
        }
    }

    return true;
}

} // namespace address
} // namespace roc
