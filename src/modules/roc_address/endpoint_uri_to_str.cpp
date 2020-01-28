/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri_to_str.h"

namespace roc {
namespace address {

endpoint_uri_to_str::endpoint_uri_to_str(const EndpointURI& u) {
    core::StringBuilder b(buf_, sizeof(buf_));

    if (!u.check(EndpointURI::Subset_Full)) {
        b.set_str("<bad>");
        return;
    }

    format_endpoint_uri(u, EndpointURI::Subset_Full, b);
}

} // namespace address
} // namespace roc
