/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_address/pct.h"
#include "roc_core/string_utils.h"

namespace roc {
namespace address {

IoURI::IoURI() {
    scheme_[0] = '\0';
    path_[0] = '\0';
}

bool IoURI::is_valid() const {
    return *scheme_ && *path_;
}

bool IoURI::is_file() const {
    return strcmp(scheme_, "file") == 0;
}

bool IoURI::is_special_file() const {
    return strcmp(scheme_, "file") == 0 && strcmp(path_, "-") == 0;
}

const char* IoURI::scheme() const {
    if (!is_valid()) {
        return "";
    }
    return scheme_;
}

const char* IoURI::path() const {
    if (!is_valid()) {
        return "";
    }
    return path_;
}

bool IoURI::set_scheme(const char* str, size_t str_len) {
    return core::copy_str(scheme_, sizeof(scheme_), str, str + str_len);
}

bool IoURI::set_encoded_path(const char* str, size_t str_len) {
    return pct_decode(path_, sizeof(path_), str, str_len) != -1;
}

bool IoURI::get_encoded_path(char* str, size_t str_len) const {
    if (!is_valid()) {
        return false;
    }
    return pct_encode(str, str_len, path_, strlen(path_), PctNonPath) != -1;
}

} // namespace address
} // namespace roc
