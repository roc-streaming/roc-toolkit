/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/network_uri.h"
#include "roc_core/panic.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace address {

namespace {

bool format_network_uri_imp(const NetworkUri& u,
                            core::StringBuilder& dst,
                            bool only_resource) {
    if (!only_resource) {
        if (!u.is_valid()) {
            return false;
        }

        if (!u.format_proto(dst)) {
            return false;
        }

        dst.append_str("://");

        if (!u.format_host(dst)) {
            return false;
        }

        if (u.port() >= 0) {
            dst.append_str(":");
            dst.append_uint((uint64_t)u.port(), 10);
        }
    }

    if (only_resource) {
        if (!u.path() && !u.encoded_query()) {
            return false;
        }
    }

    if (u.path()) {
        if (!u.format_encoded_path(dst)) {
            return false;
        }
    }

    if (u.encoded_query()) {
        dst.append_str("?");
        if (!u.format_encoded_query(dst)) {
            return false;
        }
    }

    return true;
}

} // namespace

bool format_network_uri(const NetworkUri& u, core::StringBuilder& dst) {
    return format_network_uri_imp(u, dst, false);
}

bool format_network_uri_resource(const NetworkUri& u, core::StringBuilder& dst) {
    return format_network_uri_imp(u, dst, true);
}

} // namespace address
} // namespace roc
