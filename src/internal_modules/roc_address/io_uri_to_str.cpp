/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri_to_str.h"

namespace roc {
namespace address {

io_uri_to_str::io_uri_to_str(const IoURI& u) {
    core::StringBuilder b(buf_, sizeof(buf_));

    if (!u.is_valid()) {
        b.set_str("<bad>");
        return;
    }

    format_io_uri(u, b);
}

} // namespace address
} // namespace roc
