/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_address/pct.h"

namespace roc {
namespace address {

IoURI::IoURI(core::IAllocator& allocator)
    : scheme_(allocator)
    , path_(allocator) {
}

bool IoURI::is_valid() const {
    return !scheme_.is_empty() && !path_.is_empty();
}

bool IoURI::is_file() const {
    if (!is_valid()) {
        return false;
    }
    return strcmp(scheme_.c_str(), "file") == 0;
}

bool IoURI::is_special_file() const {
    if (!is_valid()) {
        return false;
    }
    return strcmp(scheme_.c_str(), "file") == 0 && strcmp(path_.c_str(), "-") == 0;
}

void IoURI::clear() {
    scheme_.clear();
    path_.clear();
}

const char* IoURI::scheme() const {
    return scheme_.c_str();
}

const char* IoURI::path() const {
    return path_.c_str();
}

bool IoURI::set_scheme(const char* str, size_t str_len) {
    if (str_len < 1) {
        scheme_.clear();
        return false;
    }

    if (!scheme_.set_buf(str, str_len)) {
        scheme_.clear();
        return false;
    }

    return true;
}

bool IoURI::set_encoded_path(const char* str, size_t str_len) {
    if (str_len < 1) {
        path_.clear();
        return false;
    }

    if (!path_.grow(str_len + 1)) {
        path_.clear();
        return false;
    }

    core::StringBuilder b(path_.raw_buf());

    if (!pct_decode(b, str, str_len)) {
        path_.clear();
        return false;
    }

    if (!b.ok()) {
        path_.clear();
        return false;
    }

    return true;
}

bool IoURI::format_encoded_path(core::StringBuilder& dst) const {
    if (path_.is_empty()) {
        return false;
    }
    return pct_encode(dst, path_.c_str(), path_.len(), PctNonPath);
}

} // namespace address
} // namespace roc
