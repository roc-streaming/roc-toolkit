/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_uri.h"
#include "roc_core/panic.h"
#include "roc_core/string_utils.h"

namespace roc {
namespace address {

bool format_endpoint_uri(const EndpointURI& u, char* buf, size_t buf_size) {
    roc_panic_if(buf == NULL);

    if (buf_size == 0) {
        return false;
    }

    if (!u.is_valid()) {
        return false;
    }

    buf[0] = '\0';

    const char* proto = endpoint_proto_to_str(u.proto());
    if (!proto) {
        return false;
    }

    if (!core::append_str(buf, buf_size, proto)) {
        return false;
    }

    if (!core::append_str(buf, buf_size, "://")) {
        return false;
    }

    size_t pos = strlen(buf);

    if (!u.get_encoded_host(buf + pos, buf_size - pos)) {
        return false;
    }

    if (u.port() > 0) {
        if (!core::append_str(buf, buf_size, ":")) {
            return false;
        }
        if (!core::append_uint(buf, buf_size, (uint64_t)u.port(), 10)) {
            return false;
        }
    }

    if (u.path()) {
        pos = strlen(buf);

        if (!u.get_encoded_path(buf + pos, buf_size - pos)) {
            return false;
        }
    }

    if (u.encoded_query()) {
        if (!core::append_str(buf, buf_size, "?")) {
            return false;
        }
        if (!core::append_str(buf, buf_size, u.encoded_query())) {
            return false;
        }
    }

    if (u.encoded_fragment()) {
        if (!core::append_str(buf, buf_size, "#")) {
            return false;
        }
        if (!core::append_str(buf, buf_size, u.encoded_fragment())) {
            return false;
        }
    }

    return true;
}

} // namespace address
} // namespace roc
