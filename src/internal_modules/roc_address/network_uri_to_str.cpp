/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/network_uri_to_str.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace address {

network_uri_to_str::network_uri_to_str(const NetworkUri& u) {
    core::StringBuilder b(buf_, sizeof(buf_));

    if (!u.is_valid()) {
        b.rewrite("<bad>");
        return;
    }

    if (!format_network_uri(u, b)) {
        b.rewrite("<bad>");
        return;
    }
}

} // namespace address
} // namespace roc
