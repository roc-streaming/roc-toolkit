/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_core/panic.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace address {

bool format_io_uri(const IoURI& u, core::StringBuilder& dst) {
    if (!u.is_valid()) {
        return false;
    }

    dst.append_str(u.scheme());

    if (u.is_file()) {
        dst.append_str(":");
    } else {
        dst.append_str("://");
    }

    if (!u.format_encoded_path(dst)) {
        return false;
    }

    return true;
}

} // namespace address
} // namespace roc
