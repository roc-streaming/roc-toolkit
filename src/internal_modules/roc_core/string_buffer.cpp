/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <roc_core/string_buffer.h>

namespace roc {
namespace core {

StringBuffer::StringBuffer(IArena& arena)
    : data_(arena) {
    clear();
}

bool StringBuffer::is_empty() const {
    return len() == 0;
}

size_t StringBuffer::len() const {
    return data_.size() - 1;
}

const char* StringBuffer::c_str() const {
    return data_.data();
}

void StringBuffer::clear() {
    if (!data_.resize(1)) {
        roc_panic("string buffer: allocation failed");
    }
    data_[0] = '\0';
}

ROC_ATTR_NODISCARD bool StringBuffer::assign(const char* str) {
    if (!str) {
        roc_panic("string buffer: null pointer");
    }

    return assign(str, str + strlen(str));
}

ROC_ATTR_NODISCARD bool StringBuffer::assign(const char* str_begin, const char* str_end) {
    if (!str_begin || !str_end) {
        roc_panic("string buffer: null pointer");
    }
    if (str_begin > str_end) {
        roc_panic("string buffer: invalid range");
    }

    const size_t str_sz = size_t(str_end - str_begin);

    if (!data_.resize(str_sz + 1)) {
        clear();
        return false;
    }

    if (str_sz != 0) {
        memcpy(data_.data(), str_begin, str_sz);
    }
    data_[str_sz] = '\0';

    return true;
}

char* StringBuffer::extend(size_t n_chars) {
    const size_t orig_sz = data_.size();

    if (n_chars > 0) {
        if (!data_.resize(orig_sz + n_chars)) {
            clear();
            return NULL;
        }
    }

    return data_.data() + orig_sz - 1;
}

ROC_ATTR_NODISCARD bool StringBuffer::grow(size_t desired_len) {
    return data_.grow(desired_len + 1);
}

ROC_ATTR_NODISCARD bool StringBuffer::grow_exp(size_t desired_len) {
    return data_.grow_exp(desired_len + 1);
}

} // namespace core
} // namespace roc
