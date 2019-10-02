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
    if (!format_endpoint_uri(u, buf_, sizeof(buf_))) {
        strcpy(buf_, "<bad>");
    }
}

} // namespace address
} // namespace roc
