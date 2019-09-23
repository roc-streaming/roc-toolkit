/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"

namespace roc {
namespace address {

IoURI::IoURI() {
    scheme[0] = '\0';
    path[0] = '\0';
}

bool IoURI::is_empty() const {
    return !*scheme && !*path;
}

bool IoURI::is_file() const {
    return strcmp(scheme, "file") == 0;
}

bool IoURI::is_special_file() const {
    return strcmp(scheme, "file") == 0 && strcmp(path, "-") == 0;
}

} // namespace address
} // namespace roc
