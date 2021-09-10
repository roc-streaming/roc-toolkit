/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef _GNU_SOURCE
#undef _GNU_SOURCE // Ensure we're using XSI strerror_r(), not GNU one.
#endif

#include <errno.h>
#include <string.h>

#include "roc_core/errno_to_str.h"

namespace roc {
namespace core {

errno_to_str::errno_to_str() {
    format_(errno);
}

errno_to_str::errno_to_str(int err) {
    format_(err);
}

void errno_to_str::format_(int err) {
    if (strerror_r(err, buffer_, sizeof(buffer_)) != 0) {
        strcpy(buffer_, "<truncated>");
    }
}

} // namespace core
} // namespace roc
