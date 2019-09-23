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

namespace roc {
namespace address {

namespace {

bool str_append(char* buf, size_t bufsz, size_t& pos, const char* str) {
    roc_panic_if(buf == NULL);
    roc_panic_if(str == NULL);

    const size_t orig_len = strlen(str);

    size_t len = orig_len;
    if (len > bufsz - pos - 1) {
        len = bufsz - pos - 1;
    }

    if (len > 0) {
        memcpy(buf + pos, str, len);
        pos += len;
        buf[pos] = '\0';
    }

    return (len == orig_len);
}

} // namespace

bool format_io_uri(const IoURI& u, char* buf, size_t buf_size) {
    roc_panic_if(buf == NULL);

    if (buf_size == 0) {
        return false;
    }

    size_t pos = 0;

    buf[0] = '\0';

    if (*u.scheme) {
        if (!str_append(buf, buf_size, pos, u.scheme)) {
            return false;
        }

        if (u.is_file()) {
            if (!str_append(buf, buf_size, pos, ":")) {
                return false;
            }
        } else {
            if (!str_append(buf, buf_size, pos, "://")) {
                return false;
            }
        }
    }

    if (*u.path) {
        const ssize_t ret =
            pct_encode(buf + pos, buf_size - pos, u.path, strlen(u.path), PctNonPath);
        if (ret == -1) {
            return false;
        }
        pos += (size_t)ret;
    }

    roc_panic_if_not(pos <= buf_size);
    return true;
}

} // namespace address
} // namespace roc
