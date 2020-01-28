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
#include "roc_core/string_builder.h"

namespace roc {
namespace address {

bool format_endpoint_uri(const EndpointURI& u,
                         EndpointURI::Subset subset,
                         core::StringBuilder& dst) {
    if (!u.check(subset)) {
        return false;
    }

    if (subset == EndpointURI::Subset_Full) {
        dst.append_str(endpoint_proto_to_str(u.proto()));
        dst.append_str("://");

        if (!u.format_host(dst)) {
            return false;
        }

        if (u.port() > 0) {
            dst.append_str(":");
            dst.append_uint((uint64_t)u.port(), 10);
        }
    }

    if (subset == EndpointURI::Subset_Resource) {
        if (!u.path() && !u.encoded_query() && !u.encoded_fragment()) {
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

    if (u.encoded_fragment()) {
        dst.append_str("#");
        if (!u.format_encoded_fragment(dst)) {
            return false;
        }
    }

    return true;
}

} // namespace address
} // namespace roc
